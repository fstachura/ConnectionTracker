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

SQLiteLogStorage::SQLiteLogStorage(std::string path): db(path) {
    createDb();
    insertStmt = db.prepareOne(DB_INSERT_CONNECTION).first;
}

void SQLiteLogStorage::createDb() {
    db.prepareOne(DB_INIT_SQL).first.exec();
}

void SQLiteLogStorage::logConnection(ConnectionEvent event) {
    insertStmt
        .value()
        .bind(":comm", std::string(event.comm.data(), 16))
        .bind(":pid", event.pid)
        .bind(":sock_type", event.sock_type)
        //TODO
        .bind(":target_type", 0)
        //TODO store target blob 
        //.bind(":sock_type", event.sock_type)
        .bind(":target_data", nullptr)
        .bind(":target_text", event.target->to_string())
        .bind<long>(":timestamp", event.timestamp)
        .exec();

} 

SQLiteLogStorage::~SQLiteLogStorage() {}
