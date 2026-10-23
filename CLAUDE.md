# CLAUDE.md — lugha-sqlite

Behavioral instructions for AI coding assistants. Each rule exists to prevent a specific mistake.

lugha-sqlite is a Lugha package (`sqlite`, `import sqlite;`) binding SQLite's C API, so any Lugha program can keep data in a SQLite database. It is general-purpose: its first user is Rapidkoi (`../rapidkoi`, DECISION-046 and DECISION-051 there), but nothing here may know about Rapidkoi. It exists to grow Lugha's ecosystem, like `../lugha-wasmtime` and `../lugha-cbor`.

---

## SESSION HANDOFF RULE — NON-NEGOTIABLE

Every session in `CONTEXT.md` has a **name**, a **state** (`open` / `closed`) and a **branch**:

    ## SESSION {n} — {YYYY-MM-DD} — {Name} — {state}
    Branch: {branch-name}

- **Step 1 of every session, no exceptions:** append a new `open` session entry to `CONTEXT.md` (above NEXT SESSION START POINT) and commit it before reading anything else or writing code.
- Close it only after the handoff is written and committed (and pushed, once a remote exists).
- Never leave a session `open` at the end of a turn. Never start one without closing the last.
- Rewrite NEXT SESSION START POINT at the end of every session. Never delete sessions.

---

## PRP RULE — NON-NEGOTIABLE

Never write code for a new feature without an approved PRP in `/PRPs`.

1. No PRP: run `PRPs/DISCOVERY.md`, one question at a time, each with a recommended answer.
2. Write `/PRPs/prp-{NNN}-{feature_name}.md` from `PRPs/TEMPLATE.md` (next unused number, snake_case name).
3. Build only after explicit approval.

---

## SCOPE RULE — NON-NEGOTIABLE

One PRP at a time. If the scope grows mid-implementation: stop, document it, update the PRP, get approval.

**Bind what's asked, not all of SQLite.** Each PRP adds the functions it names. The C API is large; unused bindings are untested surface.

**A missing or broken lughac feature is never worked around silently.** Record it in `DECISIONS.md` and report it. It is fixed in `../lugha` under that repo's rules.

---

## EXTERN RULE — NON-NEGOTIABLE

A wrong extern is memory corruption, not a type error.

- Every `extern` is copied from the installed `sqlite3.h` (`/usr/include/sqlite3.h`, from `libsqlite3-dev`). Open the header; never write one from memory or from docs.
- The doc comment of every extern names its header: `/** … (sqlite3.h) */`.
- Ownership is stated on every extern that takes or returns a pointer: who frees it, and with which call (`sqlite3_close_v2`, `sqlite3_finalize`). A handle Lugha owns is freed exactly once.
- Bindings track one SQLite version (`docs/SQLITE_VERSION`). Changing it is a PRP that re-checks every extern against the new header.
- Text and bytes bound to a statement are passed with `SQLITE_TRANSIENT`, through `c/shim.c`, so SQLite copies them: Lugha's memory is never kept by C.
- **C shims only where an extern can't reach** (DECISION-002): variadic functions and C-only constants. Each shim function is one call into SQLite, no logic, declared in `src/raw.la` with its doc comment naming `c/shim.c`.
- Closures handed to C (the authorizer, progress handler, update hook) are pinned with `data`/`release`, as lugha-gtk does.

---

## TESTING RULE — NON-NEGOTIABLE

Write the test before the implementation.

- `@test` files live in `test/`, mirroring `src/`.
- Tests use an in-memory database (`:memory:`) or a file under `test/tmp/`, cleaned up after. No other tool is needed.
- Every public function gets a happy-path test and one per error variant.
- Run only the affected test files while working; the full suite before a merge.

---

## SECURITY RULE — NON-NEGOTIABLE

Programs using this package may run SQL that isn't their own (Rapidkoi runs its apps'). Treat SQL text and values as hostile.

- Values go in only through bound parameters. The package offers no way to paste a value into SQL text.
- The guardrails SQLite documents for untrusted SQL are exposed and documented, and examples use them: the authorizer, the progress handler (or interrupt), `SQLITE_DBCONFIG_DEFENSIVE`, `sqlite3_limit`, `sqlite3_hard_heap_limit64`, `max_page_count`.
- No extension loading. `sqlite3_load_extension` is never bound.
- Never add a `[link]` library or package dependency without stating why in the PRP.
- Never hardcode credentials or secrets.

If unsure whether something has a security implication, stop and ask.

---

## OUTWARD-FACING ACTIONS — NON-NEGOTIABLE

Ask before anything that leaves this machine: creating the GitHub repository, adding a remote, `git push`, tagging. Before any push or tag, check the test run's real exit status (`| tail` hides failures).

---

## CODE DOCUMENTATION RULE — NON-NEGOTIABLE

Read `docs/CODE_STYLE.md` before writing any function, type, or module. Every `public` item has a `/** … */` doc comment.

---

## PENDING DECISIONS RULE — NON-NEGOTIABLE

Check `DECISIONS.md` before code that depends on an architectural question. `open` blocks it; `resolved` is followed, not re-litigated; a new question is added as `open` and you stop.

---

## FILE SIZE RULE

No file exceeds 400 lines. At the limit: propose a specific split, wait for approval, then split.

---

## PROTECTED FILES — NON-NEGOTIABLE

Never read, modify, or delete:

- `.env` and `.env.*`
- `lugha.lock`: written only by lughac
- `LICENSE`: changed only by the human
- Built executables (the paths in `.gitignore`)
- SQLite's installed header and library: read `sqlite3.h`, never edit it
- `../lugha/`, `../rapidkoi/` and every other sibling repository: read for reference, never write

Keep this list, `.llmignore` and `.claude/settings.json` in sync.

---

## COMMANDS

```bash
lughac check examples/notes.la      # lex, parse, type-check
lughac build examples/notes.la      # compile and link (needs libsqlite3)
lughac test test/                   # every @test
lughac fmt --check src examples test
lughac spec                         # the Lugha reference: check it before assuming a feature exists
```

A task is done only when `fmt --check` is clean, `lughac test test/` passes and every example builds.

> Needs: `lughac` (the user installs it; never do it yourself) and SQLite's development files (`sudo apt install libsqlite3-dev`; the user installs them; never do it yourself).

---

## STACK

- **Lugha** via `lughac` 0.1.0. Use only what `lughac spec` documents.
- **SQLite 3.46.1**, the system's `libsqlite3`, pinned in `docs/SQLITE_VERSION`, linked with `[link] libs = ["sqlite3"]` (DECISION-001).
- License: **GPL-3.0-only**.

---

## ARCHITECTURE RULES

1. **`src/raw.la` (`sqlite.raw`) is the C API exactly as the header declares it.** Same names, same parameter order, handles as `extern type`. No logic. *Why:* anyone can check it against the header line by line.
2. **The entry `src/lib.la` is the safe API and never exposes a raw handle.** Errors are `Result`, every handle is freed by the package, values only through parameters. *Why:* a leaked raw handle can be freed twice or outlive its database.
3. **Only `raw` declares externs.** Safe modules import `raw`. *Why:* one file to audit against the header.
4. **A statement never outlives its database.** The safe layer ties them together. *Why:* use-after-free in C.

---

## ERROR HANDLING — NON-NEGOTIABLE

Expected failures return `Result<T, E>` with a typed error enum per module. A bad statement, a constraint, a busy or read-only database, a denied operation and an interrupted query are expected failures, never a panic. SQLite's result code and message (`sqlite3_errmsg`) are carried in the error. `panic` only for broken invariants inside this package.

---

## FILE ORGANIZATION

```
.
├── CLAUDE.md MEMORY.md CONTEXT.md DECISIONS.md CHANGELOG.md .llmignore
├── .claude/settings.json — deny rules mirroring .llmignore
├── lugha.toml            — package `sqlite`, source `src`, links `sqlite3`, compiles `c/shim.c`
├── c/shim.c              — one-call C functions for what an extern can't reach (DECISION-002), created by PRP-001
├── src/lib.la            — the entry (`import sqlite;`): `Database`, `heap_limit`, re-exporting the rest
├── src/raw.la            — `sqlite.raw`, the externs (sqlite3.h 3.46.1) and c/shim.c's three
├── src/errors.la values.la statements.la guards.la — the safe layer: errors and kinds; Value, Rows, Ran; one statement prepared, bound, stepped, finalized; the guardrails and the authorizer
├── examples/notes.la     — a guarded database in memory
├── test/                 — @test files mirroring src/
├── PRPs/                 — TEMPLATE.md, DISCOVERY.md, prp-{NNN}-….md
├── LICENSE               — GPL-3.0-only
├── docs/                 — SQLITE_VERSION, CODE_STYLE.md, source/, specs/ decisions/ incidents/ status/
└── reports/              — EOD reports
```

---

## ANTI-PATTERNS

1. **Never write an extern from memory.** Open the header.
2. **Never use a Lugha feature without checking `lughac spec`.**
3. **Never offer a way to paste a value into SQL.** Parameters only.
4. **Never add Rapidkoi-specific code.** It belongs in `../rapidkoi`.

---

## KNOWN ISSUES — DO NOT FIX

- The update hook isn't called for `WITHOUT ROWID` tables or truncate-style deletes, and the authorizer and progress handler closures are kept for the program's life (SQLite gives no release): see PRP-001's amendments.
