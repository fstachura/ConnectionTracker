#pragma once
#include <exception>
#include <iostream>
#include "sqlite3.h"
#include "ConnectionTracker.hpp"
#include "LogStorage.hpp"
#include "SQLite.hpp"

class SQLiteLogStorage: public LogStorage {
    SQLite db;
    std::optional<SQLiteStatement> insertStmt;

    void createDb();
public:
    SQLiteLogStorage(std::string path);
    virtual void logConnection(ConnectionEvent event);
    virtual ~SQLiteLogStorage();
};
