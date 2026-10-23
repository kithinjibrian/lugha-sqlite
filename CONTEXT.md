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

---

## NEXT SESSION START POINT

The owner reviews PRP-001 (revised session 2: a C shim, DECISION-002); `libsqlite3-dev` must be installed before building (every extern is copied from `/usr/include/sqlite3.h`). No remote: publishing to GitHub is asked first.
