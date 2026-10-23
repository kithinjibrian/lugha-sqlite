# CHANGELOG — lugha-sqlite

Follows [Keep a Changelog](https://keepachangelog.com) format.
Updated at the end of every session when something is completed.

---

## [Unreleased]

### Added
- The package skeleton (`lughac init --lib`) and its context system.
- PRP-001, the core bindings:
  - `Database.open` / `open_read_only`;
  - `run`, `query` with a row cap, and `batch` (one transaction, all or nothing);
  - `on_change` (the update hook);
  - `Guard.untrusted()`: an authorizer with no ATTACH, no extensions, safe PRAGMAs only and no writing the schema tables; a time limit; size, SQL and value limits; `trusted_schema` off; the defensive flag;
  - `heap_limit`.
- Values only as parameters; text and blobs copied by SQLite through `c/shim.c` (DECISION-002); one statement at a time.
- `examples/notes.la`.
