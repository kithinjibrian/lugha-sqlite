# MEMORY.md — lugha-sqlite

Resolved decisions and the project's state. Read at the start of every session.

---

## ARCHITECTURAL DECISIONS

### 1. Link the system's SQLite (DECISION-001)

**Decision:** Bind SQLite's C API, linking the system's `libsqlite3` (3.46.1 here), with its header from `libsqlite3-dev`. Bindings are checked against that header (`docs/SQLITE_VERSION`).

**Why:** It's on every Linux desktop Rapidkoi targets. Its session extension, authorizer, progress handler and heap limit are all present in this build.

**Rules out:** Vendoring SQLite's amalgamation (for now); binding from memory or docs.

### 2. Raw and safe layers (CLAUDE.md architecture rules)

**Decision:** `sqlite.raw` is the header as it is; the entry is a safe API with `Result`s, owned handles and parameters only.

**Why:** One file to audit against the header; nothing unsafe reaches callers.

### 3. A C shim, only where an extern can't reach (DECISION-002)

**Decision:** `c/shim.c` holds one-call C functions for what Lugha can't declare: the variadic `sqlite3_db_config` and `SQLITE_TRANSIENT`. Everything else is a Lugha extern.

**Why:** Lugha supports no variadic externs and can't make pointers; it recommends shims (its CONTEXT.md).

---

## CURRENT PROJECT STATE

### Fully Working
- The context system (session 1).
- PRP-001, the core bindings: open, run, query, batch, change notices, the guardrails (sessions 3–4). 16 tests pass. Published at github.com/kithinjibrian/lugha-sqlite.

### In Progress
- Nothing.

### Not Started
- The session extension (changesets), backups, user functions, incremental blobs.

---

## NEXT SESSION START POINT

See CONTEXT.md.
