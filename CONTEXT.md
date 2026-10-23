# CONTEXT.md — lugha-sqlite

The session handoff log. Every session is appended here; none is ever deleted.

---

## SESSION 1 — 2026-10-24 — The package, its context system, PRP-001 — closed

Branch: main

### WHAT WAS DONE

- Started from Rapidkoi's storage decisions (its DECISION-046, SQL-first; DECISION-051, this package first), at the owner's word ("package first then storage prp"; PRP-001's scope: "yes, that scope works").
- `lughac init --lib --name sqlite`, then the context system modelled on lugha-wasmtime:
  - CLAUDE.md (extern, security and guardrail rules for SQL that isn't the caller's);
  - MEMORY.md, DECISIONS.md (DECISION-001: the system's libsqlite3 3.46.1);
  - CHANGELOG.md, README.md, `.llmignore`, `.claude/settings.json`;
  - `docs/SQLITE_VERSION`, `docs/CODE_STYLE.md`, `PRPs/TEMPLATE.md` and `DISCOVERY.md`;
  - `LICENSE` (GPL-3.0-only, as the siblings).
- Checked on this machine: `libsqlite3.so.0` exports the session extension, authorizer, progress handler, update hook and heap limit. `sqlite3.h` isn't installed yet (`libsqlite3-dev`, the owner's to install).
- Wrote `PRPs/prp-001-core_bindings.md`. It raises two questions (variadic `sqlite3_db_config`, `SQLITE_TRANSIENT`) for the owner. No code.

## SESSION 2 — 2026-10-24 — PRP-001 revised: a C shim for what Lugha can't declare — closed

Branch: main

### WHAT WAS DONE

- The owner: Lugha's plan is no variadics, so C shims instead. Confirmed in `../lugha` (its CONTEXT.md: "recommended C shims for now"; spec section 6: `[link] c = [...]`).
- PRP-001 revised: `c/shim.c` with three one-call functions (`db_config` for the defensive flag, and binding text and blobs with `SQLITE_TRANSIENT`). Its two questions are gone.
- DECISION-002 recorded; MEMORY.md decision 3; CLAUDE.md's extern rule and tree updated. No code.

## SESSION 3 — 2026-10-24 — Build PRP-001: the core bindings — closed

Branch: main

### WHAT WAS DONE

- The owner: "yes approved, build it"; `libsqlite3-dev` installed.
- Tests first: `test/database.la`, `test/guards.la`.
- `src/raw.la`: externs copied from `/usr/include/sqlite3.h` 3.46.1, constants, and the shim's three.
- `c/shim.c` (`lsqlite_db_config_flag`, `lsqlite_bind_text`, `lsqlite_bind_blob`), linked through `[link] c`.
- The safe layer: `errors.la`, `values.la`, `statements.la`, `guards.la`, `lib.la` (`Database`, `heap_limit`).
- `examples/notes.la` replaces `hello.la`. It builds, so the shim and libsqlite3 link.
- Checks: fmt is clean; both test files check. Tests not run (the owner runs them).

---

## NEXT SESSION START POINT

PRP-001 is built (session 3), awaiting the owner's run of `lughac test test/` and `lughac run examples/notes.la`. Then Rapidkoi's storage PRP uses this package. That needs it published to GitHub first (Rapidkoi takes sibling packages by git URL), which is asked first. No remote yet.
