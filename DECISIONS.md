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

### DECISION-001 — Which SQLite, and how it's linked

**Status:** resolved
**Raised:** 2026-10-24 — Session 1
**Resolved:** 2026-10-24 — Session 1 (the owner, in Rapidkoi's session 207: "package first then storage prp"; scope: "yes, that scope works")

**Question:** Bind which SQLite: the system's, or a vendored copy?

**Outcome:** The system's `libsqlite3` (3.46.1-9ubuntu0.3 here), linked with `[link] libs = ["sqlite3"]`, its header from `libsqlite3-dev` (the owner installs it). Every extern is checked against that header.

**Rationale:** It's present on the desktops Rapidkoi targets, and this build has everything needed (session extension, authorizer, progress handler, heap limit), checked with `nm -D`. Vendoring can come later if a platform lacks it.

**Copied to MEMORY.md:** yes (decision 1)
