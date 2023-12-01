#include <iostream>
#include <thread>
#include "ConnectionTracker.hpp"
#include "sqlite3.h"
#include "SQLiteLogStorage.hpp"

const char DB_INIT_SQL[] = R"""(
    CREATE TABLE IF NOT EXISTS connection_log (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        comm STRING CHECK(LENGTH(comm) <= 16),
        pid int,
        sock_type INT,
        target_type INT,
        target_data BLOB,
        target_text STRING,
        timestamp INT
    );
)""";

const char DB_INSERT_CONNECTION[] = R"""(
    INSERT INTO connection_log (
        comm, pid, sock_type, target_type, target_text, target_data, timestamp
    ) VALUES (
        :comm, :pid, :sock_type, :target_type, :target_text, :target_data, :timestamp 
    );
)""";

SQLiteException::SQLiteException(int errcode, std::string msg): errcode(errcode), msg(msg) { }

const char* SQLiteException::what() const noexcept {
    return msg.c_str();
}

int SQLiteException::getErrcode() const noexcept {
    return errcode;
}

SQLiteLogStorage::SQLiteLogStorage(std::string path) {
    checkSqliteErr(sqlite3_open(path.c_str(), &db));
    createDb();
    checkSqliteErr(sqlite3_prepare_v2(db, DB_INSERT_CONNECTION, sizeof(DB_INSERT_CONNECTION), &insert_stmt, NULL));
}

int SQLiteLogStorage::checkSqliteErr(int result) {
    if(result != SQLITE_OK && result != SQLITE_DONE) {
        throw SQLiteException(result, sqlite3_errmsg(db));
    }
    return result;
}

int SQLiteLogStorage::stepUntilDone(sqlite3_stmt* stmt) {
    while(true) {
        int result = sqlite3_step(stmt);
        if(result == SQLITE_OK || result == SQLITE_DONE || result == SQLITE_EMPTY) {
            return result;
        }
        checkSqliteErr(result);
    }
}

void SQLiteLogStorage::createDb() {
    sqlite3_stmt* stmt;
    checkSqliteErr(sqlite3_prepare_v2(db, DB_INIT_SQL, sizeof(DB_INIT_SQL), &stmt, NULL));
    stepUntilDone(stmt);
    checkSqliteErr(sqlite3_finalize(stmt));
}

void SQLiteLogStorage::logConnection(ConnectionEvent event) {
    checkSqliteErr(sqlite3_reset(insert_stmt));
    checkSqliteErr(sqlite3_bind_text(
        insert_stmt, 
        sqlite3_bind_parameter_index(insert_stmt, ":comm"),
        event.comm.data(),
        event.comm.size(),
        NULL
    ));
    checkSqliteErr(sqlite3_bind_int64(
        insert_stmt, 
        sqlite3_bind_parameter_index(insert_stmt, ":pid"),
        event.pid
    ));
    checkSqliteErr(sqlite3_bind_int(
        insert_stmt, 
        sqlite3_bind_parameter_index(insert_stmt, ":sock_type"),
        event.sock_type
    ));
    checkSqliteErr(sqlite3_bind_int(
        insert_stmt, 
        sqlite3_bind_parameter_index(insert_stmt, ":target_type"),
        0
    ));
    checkSqliteErr(sqlite3_bind_zeroblob(
        insert_stmt, 
        sqlite3_bind_parameter_index(insert_stmt, ":target_data"),
        0
    ));
    //:target_type
    //checkSqliteErr(sqlite3_bind_int(
    //    insert_stmt, 
    //    sqlite3_bind_parameter_index(insert_stmt, "sock_type"),
    //    event.sock_type
    //));
    //TODO store target blob too
    auto target_str = event.target->to_string();
    checkSqliteErr(sqlite3_bind_text(
        insert_stmt, 
        sqlite3_bind_parameter_index(insert_stmt, ":target_text"),
        target_str.c_str(),
        target_str.size(),
        NULL
    ));
    checkSqliteErr(sqlite3_bind_int64(
        insert_stmt, 
        sqlite3_bind_parameter_index(insert_stmt, ":timestamp"),
        event.timestamp
    ));
    stepUntilDone(insert_stmt);
} 

SQLiteLogStorage::~SQLiteLogStorage() {
    sqlite3_close(db);
    sqlite3_finalize(insert_stmt);
}
