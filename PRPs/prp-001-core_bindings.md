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
  - `src/statements.la`, `src/guards.la` as the safe layer grows (each under 400 lines).
  - `examples/notes.la`, replacing `hello.la`.
  - Tests in `test/`.
- **SQLite C functions bound** (sqlite3.h, 3.46.1):
  - **Open and close:** `sqlite3_open_v2`, `sqlite3_close_v2`.
  - **Errors:** `sqlite3_errmsg`, `sqlite3_extended_errcode`, `sqlite3_errstr`.
  - **Statements:**
    - prepare, step, reset, finalize: `sqlite3_prepare_v2`, `sqlite3_step`, `sqlite3_reset`, `sqlite3_clear_bindings`, `sqlite3_finalize`;
    - binding: `sqlite3_bind_parameter_count`, `sqlite3_bind_int64`, `sqlite3_bind_double`, `sqlite3_bind_text`, `sqlite3_bind_blob`, `sqlite3_bind_null`;
    - reading columns: `sqlite3_column_count`, `sqlite3_column_name`, `sqlite3_column_type`, `sqlite3_column_int64`, `sqlite3_column_double`, `sqlite3_column_text`, `sqlite3_column_blob`, `sqlite3_column_bytes`;
    - `sqlite3_changes64`, `sqlite3_last_insert_rowid`.
  - **Guardrails:** `sqlite3_set_authorizer`, `sqlite3_progress_handler`, `sqlite3_interrupt`, `sqlite3_limit`, `sqlite3_hard_heap_limit64`, plus `PRAGMA max_page_count` and `PRAGMA trusted_schema = OFF` through plain SQL.
  - **Change notices:** `sqlite3_update_hook`.
  - **Transactions:** plain SQL (`BEGIN`, `COMMIT`, `ROLLBACK`); `Database.batch` runs several statements in one, rolled back on any failure.
- **Ownership:**
  - A `Database` owns its `sqlite3*`, closed once with `sqlite3_close_v2`. Its statements are finalized first, so close never fails for them.
  - A statement is finalized after each `run` or `query` (prepared again next time); caching comes later if needed.
  - Text from C (`sqlite3_column_text`, `errmsg`) is copied by Lugha (spec section 8). Blobs are copied with `c_read` after `sqlite3_column_bytes`.
  - Closures handed to C are pinned with `data` (spec section 8) and live as long as their database.
- **Lugha spec sections relied on:** section 8 (externs, `out`/`ptr`/`mut` parameters, callbacks with `data`, `CPtr`, `c_read`), `Result`, enums. Each is confirmed while building.
- Open decisions that must be resolved first: the two questions below.

## QUESTIONS FOR THE OWNER (found while writing this)

1. **SQLite's defensive flag needs a C function Lugha can't declare.**
   - `SQLITE_DBCONFIG_DEFENSIVE` is set through `sqlite3_db_config`, which is variadic. Lugha's spec says variadic C functions can't be declared.
   - Options:
     - (a) Ask for variadic externs in `../lugha` (an upstream change, under its rules), and bind the flag once it lands.
     - (b) Go without the flag for now. The authorizer denies what it guards against: writing the schema by hand (`PRAGMA writable_schema`), raw page access (`sqlite_dbpage`), and the schema tables. `trusted_schema = OFF` is set by SQL.
   - **Recommended: (b) now, and (a) asked upstream,** recorded as DECISION-002, never worked around in secret.
2. **SQLite can't be told to copy a bound string.**
   - That's `SQLITE_TRANSIENT`, the pointer value −1, which Lugha can't make (a pointer only comes from C).
   - Options:
     - (a) `SQLITE_STATIC` (`NULL`): SQLite reads the value in place. The package keeps every bound value referenced until the statement is reset or finalized, so Boehm never frees it while SQLite may read it.
     - (b) Ask upstream for a way to make that one constant.
   - **Recommended: (a).** It's how SQLite is meant to be used for values that outlive the step, and the safe layer guarantees it. Recorded as DECISION-003. CLAUDE.md's extern rule is updated to match.

## IMPLEMENTATION REQUIREMENTS

### Must Do
- Every extern copied from `sqlite3.h`, its doc comment naming it.
- Values only through parameters: `run` and `query` take `Value[]`, and there's no API that pastes text into SQL.
- **`Guard.untrusted()` applies SQLite's guidance:**
  - an authorizer allowing reads, writes and schema changes on the main database, and denying `ATTACH`/`DETACH`, extension loading, `PRAGMA`s outside a short safe list, and the schema tables and `sqlite_dbpage`;
  - a progress handler stopping a statement past a time limit (default 1 s);
  - `sqlite3_limit` lowering SQL length, columns, attached databases (0) and so on;
  - `max_page_count` for size;
  - `trusted_schema = OFF`.
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
- Callbacks are pinned for their database's life and never called after it closes (the hooks are cleared before `sqlite3_close_v2`).

## TESTS TO WRITE

Against `:memory:` or `test/tmp/`:
- [ ] Open, create, insert, query: columns and each value type (integer, real, text, blob, null) round-trip.
- [ ] Parameters: each type binds. A value with quotes in it is stored as text, never run as SQL.
- [ ] Errors: a syntax error, a constraint, a read-only file (`Database.open_read_only`), each with its kind and message.
- [ ] `batch`: all or nothing; a failing statement rolls back the earlier ones.
- [ ] Guardrails:
  - `ATTACH` and a denied `PRAGMA` fail as `Denied`;
  - a recursive query past the time limit fails as `Interrupted`;
  - inserting past `max_page_count` fails as `Full`;
  - SQL longer than the limit fails as `TooBig`;
  - `max_rows` cuts.
- [ ] `on_change` reports the table and rowid of inserts, updates and deletes.
- [ ] Closing with a hook set, and closing twice, are safe.

## ROLLBACK PLAN
- Branch: `main`; the package is new, so revert to the skeleton.

## ACCEPTANCE CRITERIA
- [ ] Every extern checked against `/usr/include/sqlite3.h` (3.46.1).
- [ ] `lughac test test/` passes (the owner runs it); `examples/notes.la` builds.
- [ ] `lughac fmt --check src examples test` is clean; every public item documented; no file over 400 lines.
- [ ] CHANGELOG.md updated; DECISION-002 and DECISION-003 recorded.

## VALIDATION
- `lughac fmt --check src examples test`, `lughac test test/`, `lughac build examples/notes.la`
