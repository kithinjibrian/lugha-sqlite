/*
 * lugha-sqlite's C shim (DECISION-002): only what a Lugha extern can't
 * reach. sqlite3_db_config is variadic, and SQLITE_TRANSIENT is the
 * pointer -1, which Lugha can't make. Each function is one call into
 * SQLite and nothing else; src/raw.la declares them.
 */
#include <sqlite3.h>

/* Sets an on/off database option, such as SQLITE_DBCONFIG_DEFENSIVE. */
int lsqlite_db_config_flag(sqlite3 *db, int op, int on) {
    return sqlite3_db_config(db, op, on, (int *)0);
}

/* Binds text, which SQLite copies (SQLITE_TRANSIENT). */
int lsqlite_bind_text(sqlite3_stmt *stmt, int index, const char *text, int bytes) {
    return sqlite3_bind_text(stmt, index, text, bytes, SQLITE_TRANSIENT);
}

/* Binds bytes, which SQLite copies (SQLITE_TRANSIENT). */
int lsqlite_bind_blob(sqlite3_stmt *stmt, int index, const void *data, int bytes) {
    return sqlite3_bind_blob(stmt, index, data, bytes, SQLITE_TRANSIENT);
}
