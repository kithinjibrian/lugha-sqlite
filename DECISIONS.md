# DECISIONS.md — lugha-sqlite

Tracks architectural and design questions that are open, deferred, or resolved.

Rules:
- Every open decision blocks implementation of the code it affects.
- The AI must not implement anything that depends on an open decision.
- When a decision is resolved, move it to the RESOLVED section and record the outcome.
- Once resolved, copy the outcome to MEMORY.md as an architectural decision.

---

## OPEN — Requires human input before implementation

(none)

---

## DEFERRED — Acknowledged, not yet needed

(none)

---

## RESOLVED

### DECISION-002 — A C shim for what a Lugha extern can't reach

**Status:** resolved
**Raised:** 2026-10-24 — Session 1 (writing PRP-001)
**Resolved:** 2026-10-24 — Session 2 (the owner: "the plan was not to support variadics. i think you need to write c shims for that")

**Question:** `sqlite3_db_config` (the defensive flag) is variadic, and `SQLITE_TRANSIENT` is the pointer −1. Lugha can declare neither. How are they reached?

**Outcome:** A C shim, `c/shim.c`, compiled through `[link] c = [...]`. It holds one-call functions only: `lsqlite_db_config_flag`, `lsqlite_bind_text`, `lsqlite_bind_blob`. Every other binding is a Lugha extern. A new shim function needs a PRP saying why an extern can't do it.

**Rationale:** Lugha's own decision (its CONTEXT.md: variadic externs, "recommended C shims for now; no PRP"); lughac supports package C files (milestone 87).

**Copied to MEMORY.md:** yes (decision 3)

---

### DECISION-001 — Which SQLite, and how it's linked

**Status:** resolved
**Raised:** 2026-10-24 — Session 1
**Resolved:** 2026-10-24 — Session 1 (the owner, in Rapidkoi's session 207: "package first then storage prp"; scope: "yes, that scope works")

**Question:** Bind which SQLite: the system's, or a vendored copy?

**Outcome:** The system's `libsqlite3` (3.46.1-9ubuntu0.3 here), linked with `[link] libs = ["sqlite3"]`, its header from `libsqlite3-dev` (the owner installs it). Every extern is checked against that header.

**Rationale:** It's present on the desktops Rapidkoi targets, and this build has everything needed (session extension, authorizer, progress handler, heap limit), checked with `nm -D`. Vendoring can come later if a platform lacks it.

**Copied to MEMORY.md:** yes (decision 1)
