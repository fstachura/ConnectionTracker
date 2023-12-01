#pragma once
#include <exception>
#include <string>
#include "sqlite3.h"

class SQLiteException: public std::exception {
    std::string msg;
    int errcode;
public:
    SQLiteException(int errcode, std::string msg);
    virtual const char* what() const noexcept;
    int getErrcode() const noexcept;
};

class SQLiteStatement {
};

class SQLite {
    sqlite3* db = nullptr;

    int checkSqliteErr(int result);
    int stepUntilDone(sqlite3_stmt* stmt);
public:
    SQLite(std::string path);
    virtual ~SQLite();
};
