#include "SQLite.hpp"

SQLite::SQLite(std::string path) {
    checkSqliteErr(sqlite3_open(path.c_str(), &db));
}

int SQLite::checkSqliteErr(int result) {
    if(result != SQLITE_OK && result != SQLITE_DONE) {
        throw SQLiteException(result, sqlite3_errmsg(db));
    }
    return result;
}

int SQLite::stepUntilDone(sqlite3_stmt* stmt) {
    while(true) {
        int result = sqlite3_step(stmt);
        if(result == SQLITE_OK || result == SQLITE_DONE || result == SQLITE_EMPTY) {
            return result;
        }
        checkSqliteErr(result);
    }
}

SQLite::~SQLite() {
    sqlite3_close(db);
}
