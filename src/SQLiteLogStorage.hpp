#pragma once
#include <exception>
#include <iostream>
#include "sqlite3.h"
#include "ConnectionTracker.hpp"
#include "LogStorage.hpp"
#include "SQLite.hpp"

class SQLiteLogStorage: public LogStorage {
    sqlite3* db = nullptr;
    sqlite3_stmt* insert_stmt = nullptr;

    void createDb();
    int checkSqliteErr(int result);
    int stepUntilDone(sqlite3_stmt* stmt);
public:
    SQLiteLogStorage(std::string path);
    virtual void logConnection(ConnectionEvent event);
    virtual ~SQLiteLogStorage();
};
