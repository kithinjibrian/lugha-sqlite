## FEATURE: [one sentence]

## OBJECTIVE
[2–3 sentences: what a Lugha program can do when this is done, with a short `.la` example]

## CONTEXT

- Starting state / ending state: [files]
- sqlite C functions bound: [each with its header and the pinned sqlite version]
- Ownership: [for each pointer: who owns it, which `sqlite3_*` call frees it (`sqlite3_close_v2`, `sqlite3_finalize`)]
- Lugha spec sections relied on: [confirm each feature exists]
- Open decisions that must be resolved first: [DECISIONS.md entries]

## IMPLEMENTATION REQUIREMENTS

### Must Do
- [specific requirement]

### Must NOT Do
- [explicit exclusion, and why]

## ERROR HANDLING REQUIREMENTS

- [Error enum variants, and which SQLite result codes map to each]
- [Conditions that panic (broken invariants only), and the message]

## SECURITY CONSIDERATIONS

- [SQL from untrusted callers: which guardrails apply (authorizer, progress handler, limits, defensive flag)]
- [Values only through bound parameters, never pasted into SQL text]
- [Callbacks into Lugha: what C may call, and when they're released]
- [New `[link]` or package dependencies, and why]

## TESTS TO WRITE

Each against an in-memory database (`:memory:`) or a file under `test/tmp/`:
- [ ] Happy path: [describe]
- [ ] Error path: [each variant: a bad statement, a constraint, a busy or read-only file, a denied operation, an interrupted query]
- [ ] Edge case: [describe]

## ROLLBACK PLAN
- Branch to return to: [branch]
- State the codebase should be in: [describe]

## ACCEPTANCE CRITERIA
- [ ] [testable criterion]
- [ ] Every extern checked against the pinned headers
- [ ] `lughac test test/` passes; every example builds
- [ ] `lughac fmt --check src examples test` is clean
- [ ] Every `public` item has a `/** … */` doc comment
- [ ] No file over 400 lines
- [ ] CHANGELOG.md updated

## VALIDATION
- `lughac fmt --check src examples test`
- `lughac test test/`
- `for f in examples/*.la; do lughac build "$f" || exit 1; done`
