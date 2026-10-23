# lugha-sqlite

SQLite for [Lugha](https://github.com/kithinjibrian/lugha): `import sqlite;`.

A safe API over SQLite's C API: open a database, run SQL with bound parameters, read rows, and the guardrails SQLite documents for running SQL you didn't write (an authorizer, a time limit, size and memory limits). Values only ever go in as parameters.

## Needs

- `lughac`
- SQLite's development files: `sudo apt install libsqlite3-dev`

## Status

Early: the bindings are being written (PRP-001).

## License

GPL-3.0-only.
