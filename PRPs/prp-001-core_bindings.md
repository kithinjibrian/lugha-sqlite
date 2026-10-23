## FEATURE: The core bindings: open a database, run SQL with bound parameters and read its rows, with the guardrails SQLite documents for SQL a program didn't write.

## OBJECTIVE
A Lugha program can keep data in SQLite safely, including SQL from someone else (Rapidkoi runs its apps' SQL):

```
import sqlite { Database, Value, Guard };

let db = Database.open("notes.db")?;              // read-write, created if missing
db.guard(Guard.untrusted());                       // the authorizer, a time limit, size and memory limits
db.run("CREATE TABLE IF NOT EXISTS notes (id INTEGER PRIMARY KEY, text TEXT)", [])?;
db.run("INSERT INTO notes (text) VALUES (?)", [Value.Text("Buy milk")])?;
let rows = db.query("SELECT id, text FROM notes WHERE id > ?", [Value.Integer(0)])?;
// rows.columns == ["id", "text"]; rows.values[0] == [Value.Integer(1), Value.Text("Buy milk")]
db.on_change(fun(table: string, rowid: i64) = …);  // the update hook
db.close();
```

## CONTEXT

- Starting state: the `lughac init --lib` skeleton (`src/lib.la`, `examples/hello.la`).
- Ending state:
  - **`src/raw.la` (`sqlite.raw`):** the externs, copied from `/usr/include/sqlite3.h` (3.46.1).
  - **`src/lib.la`:** the entry, `Database`, `Value`, `Rows`, `Guard`, `SqliteError`.
  - **`c/shim.c`:** the three C functions below.
  - `src/statements.la`, `src/guards.la` as the safe layer grows (each under 400 lines).
  - `examples/notes.la`, replacing `hello.la`.
  - Tests in `test/`.
- **SQLite C functions bound** (sqlite3.h, 3.46.1):
  - **Open and close:** `sqlite3_open_v2`, `sqlite3_close_v2`.
  - **Errors:** `sqlite3_errmsg`, `sqlite3_extended_errcode`, `sqlite3_errstr`.
  - **Statements:**
    - prepare, step, reset, finalize: `sqlite3_prepare_v2`, `sqlite3_step`, `sqlite3_reset`, `sqlite3_clear_bindings`, `sqlite3_finalize`;
    - binding: `sqlite3_bind_parameter_count`, `sqlite3_bind_int64`, `sqlite3_bind_double`, `sqlite3_bind_null`, and text and blobs through the shim (`SQLITE_TRANSIENT`);
    - reading columns: `sqlite3_column_count`, `sqlite3_column_name`, `sqlite3_column_type`, `sqlite3_column_int64`, `sqlite3_column_double`, `sqlite3_column_text`, `sqlite3_column_blob`, `sqlite3_column_bytes`;
    - `sqlite3_changes64`, `sqlite3_last_insert_rowid`.
  - **Guardrails:** `SQLITE_DBCONFIG_DEFENSIVE` (through the shim), `sqlite3_set_authorizer`, `sqlite3_progress_handler`, `sqlite3_interrupt`, `sqlite3_limit`, `sqlite3_hard_heap_limit64`, plus `PRAGMA max_page_count` and `PRAGMA trusted_schema = OFF` through plain SQL.
  - **Change notices:** `sqlite3_update_hook`.
  - **Transactions:** plain SQL (`BEGIN`, `COMMIT`, `ROLLBACK`); `Database.batch` runs several statements in one, rolled back on any failure.
- **Ownership:**
  - A `Database` owns its `sqlite3*`, closed once with `sqlite3_close_v2`. Its statements are finalized first, so close never fails for them.
  - A statement is finalized after each `run` or `query` (prepared again next time); caching comes later if needed.
  - Text from C (`sqlite3_column_text`, `errmsg`) is copied by Lugha (spec section 8). Blobs are copied with `c_read` after `sqlite3_column_bytes`.
  - Closures handed to C are pinned with `data` (spec section 8) and live as long as their database.
- **Lugha spec sections relied on:** section 8 (externs, `out`/`ptr`/`mut` parameters, callbacks with `data`, `CPtr`, `c_read`), `Result`, enums. Each is confirmed while building.
- Open decisions that must be resolved first: none (DECISION-002, the shim, resolved by the owner).

## A C SHIM FOR WHAT LUGHA CAN'T DECLARE (the owner, session 2; DECISION-002)

Two pieces of SQLite's API can't be reached from a Lugha `extern`:
- `sqlite3_db_config`, which sets `SQLITE_DBCONFIG_DEFENSIVE`, is variadic, and Lugha has no variadic externs.
- `SQLITE_TRANSIENT`, which tells SQLite to copy a bound string or blob, is the pointer value −1, and Lugha can't make a pointer.

Lugha's own answer is a C shim (`../lugha` CONTEXT.md: variadic externs, "recommended C shims for now"). lughac compiles a package's C files into every program using it (`[link] c = […]`, spec section 6). So:
- **`c/shim.c` (new):** three functions, each one call into SQLite and nothing else:
  - `int lsqlite_db_config_flag(sqlite3 *db, int op, int on)` → `sqlite3_db_config(db, op, on, (int *)0)`;
  - `int lsqlite_bind_text(sqlite3_stmt *s, int i, const char *text, int n)` → `sqlite3_bind_text(s, i, text, n, SQLITE_TRANSIENT)`;
  - `int lsqlite_bind_blob(sqlite3_stmt *s, int i, const void *bytes, int n)` → `sqlite3_bind_blob(s, i, bytes, n, SQLITE_TRANSIENT)`.
- **`lugha.toml`:** `[link] c = ["c/shim.c"]`.
- **`src/raw.la`** declares the three as externs, beside SQLite's own, their doc comments naming `c/shim.c`.
- SQLite copies every bound text and blob, so Lugha's memory is never kept by C, as CLAUDE.md's extern rule says. The defensive flag is set by `Guard.untrusted()`.

## IMPLEMENTATION REQUIREMENTS

### Must Do
- Every extern copied from `sqlite3.h`, its doc comment naming it.
- Values only through parameters: `run` and `query` take `Value[]`, and there's no API that pastes text into SQL.
- **`Guard.untrusted()` applies SQLite's guidance:**
  - an authorizer allowing reads, writes and schema changes on the main database, and denying `ATTACH`/`DETACH`, extension loading, `PRAGMA`s outside a short safe list, and the schema tables and `sqlite_dbpage`;
  - a progress handler stopping a statement past a time limit (default 1 s);
  - `sqlite3_limit` lowering SQL length, columns, attached databases (0) and so on;
  - `max_page_count` for size;
  - `trusted_schema = OFF`;
  - the defensive flag.
  - Each is a field a caller can set, with documented defaults.
- `query` stops at a row cap the caller gives (`max_rows`) and reports that it was cut.
- `sqlite3_hard_heap_limit64` is process-wide: exposed as `sqlite.heap_limit(bytes)`, documented as such, not per database.

### Must NOT Do
- The session extension, backups, user functions, incremental blobs, `sqlite3_load_extension`.
- Anything Rapidkoi-specific.
- A remote, or a push (asked first).

## ERROR HANDLING REQUIREMENTS
- `SqliteError { code: i64, extended: i64, message: string }` with a kind: `Busy`, `Constraint`, `ReadOnly`, `Denied` (authorizer), `Interrupted` (time), `TooBig`, `Full` (size), `Misuse`, `Other`.
- A failure in `batch` rolls it back before returning the error.
- `panic` only for broken invariants (a handle freed twice).

## SECURITY CONSIDERATIONS
- Untrusted SQL is this package's reason for guardrails; they're tested one by one.
- **New `[link]` dependency: `sqlite3`** (the system's libsqlite3), the package's whole purpose.
- **C code in the package: `c/shim.c`.** Owner review: three functions, each one call, no logic. Every other binding stays a Lugha extern.
- Callbacks are pinned for their database's life and never called after it closes (the hooks are cleared before `sqlite3_close_v2`).

## TESTS TO WRITE

Against `:memory:` or `test/tmp/`:
- [x] Open, create, insert, query: columns and each value type (integer, real, text, blob, null) round-trip.
- [x] Parameters: each type binds. A value with quotes in it is stored as text, never run as SQL. Text and blobs are copied: changing the Lugha value after binding changes nothing stored.
- [x] Errors: a syntax error, a constraint, a read-only file (`Database.open_read_only`), each with its kind and message.
- [x] `batch`: all or nothing; a failing statement rolls back the earlier ones.
- [x] Guardrails:
  - `ATTACH` and a denied `PRAGMA` fail as `Denied`;
  - a recursive query past the time limit fails as `Interrupted`;
  - inserting past `max_page_count` fails as `Full`;
  - SQL longer than the limit fails as `TooBig`;
  - with the defensive flag on, `PRAGMA writable_schema = ON` has no effect;
  - `max_rows` cuts.
- [x] `on_change` reports the table and rowid of inserts, updates and deletes.
- [x] Closing with a hook set, and closing twice, are safe.

## ROLLBACK PLAN
- Branch: `main`; the package is new, so revert to the skeleton.

## ACCEPTANCE CRITERIA
- [x] Every extern checked against `/usr/include/sqlite3.h` (3.46.1).
- [ ] `lughac test test/` passes (the owner runs it); `examples/notes.la` builds.
- [ ] `lughac fmt --check src examples test` is clean; every public item documented; no file over 400 lines.
- [x] CHANGELOG.md updated; DECISION-002 (the shim) recorded.

## VALIDATION
- `lughac fmt --check src examples test`, `lughac test test/`, `lughac build examples/notes.la`

## AMENDMENTS (session 3, while building)

- **The shim's functions are `lsqlite_…`,** not `lugha_sqlite_…`: lughac reserves names starting with `lugha_` (E0306).
- **One statement per call is checked:** after preparing, the rest of the SQL must be only spaces and `;`, or the call fails as `Misuse` ("Only one statement at a time"), so a second statement can't ride along.
- **Text is read as bytes** (`sqlite3_column_text` then `sqlite3_column_bytes`, copied with `c_read`), so text holding a NUL comes back whole. Text that isn't UTF-8 (a blob cast to text) comes back as `Bytes`.
- **The authorizer also denies any database but `main` and `temp`,** and matches names without case (ASCII), since SQLite passes them as written.
- **Hooks aren't cleared before close.** SQLite never calls them after `sqlite3_close_v2`, and a callback parameter can't be passed as `NULL` from Lugha. The authorizer's and progress handler's closures are kept for the program's life: SQLite gives them no release.
- **The time limit** is a progress handler checked every 1000 steps against a deadline each call sets (`run`, `query`, `batch`).
- **Tests added:** the defensive flag on its own (with the authorizer off), and `decide` action by action.

