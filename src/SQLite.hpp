#pragma once
#include <cstddef>
#include <exception>
#include <string>
#include <memory>
#include <vector>
#include <optional>
#include "sqlite3.h"

class SQLiteException: public std::exception {
    std::string msg;
    int errcode;
public:
    SQLiteException(int errcode, std::string msg);
    SQLiteException(std::string msg);
    virtual const char* what() const noexcept;
    int getErrcode() const noexcept;
};

class SQLiteStatement;

class SQLite {
    std::shared_ptr<sqlite3> db;

    int checkSqliteErr(int result);
    int stepUntilDone(sqlite3_stmt* stmt);
public:
    SQLite(std::string path);
    std::vector<SQLiteStatement> prepare(std::string stmt);
    std::pair<SQLiteStatement, std::string_view> prepareOne(std::string_view view);
};

class SQLiteResult;

class SQLiteStatement {
    std::shared_ptr<sqlite3> db;
    std::shared_ptr<sqlite3_stmt> stmt;
    bool gettingResults = false;

    friend class SQLite; 
    SQLiteStatement(std::shared_ptr<sqlite3> db, std::shared_ptr<sqlite3_stmt> stmt);
    int checkSqliteErr(int result);
    void checkIfBusy();
public:
    template<typename T>
    SQLiteStatement& bind(std::string param, T data) {
        int index = sqlite3_bind_parameter_index(stmt.get(), param.c_str());
        if(index == 0) {
            throw SQLiteException("failed to find parameter " + param);
        }
        return bind(index, data);
    }

    SQLiteStatement& bind(int param, const std::vector<unsigned char>& data);
    SQLiteStatement& bind(int param, const std::string_view data);
    SQLiteStatement& bind(int param, int value);
    SQLiteStatement& bind(int param, long value);
    SQLiteStatement& bind(int param, double value);
    SQLiteStatement& bind(int param, std::nullptr_t);
    void exec();
    SQLiteResult getResults();
};

class SQLiteResult {
    std::shared_ptr<sqlite3_stmt> stmt;

    friend class SQLiteStatement; 
    SQLiteResult(std::shared_ptr<sqlite3_stmt> stmt);
    int checkSqliteErr(int result);
public:
    bool next();

    std::optional<std::string> getString(int col);
    std::optional<std::vector<unsigned char>> getBlob(int col);
    std::optional<int> getInt(int col);
    std::optional<long> getLong(int col);
    std::optional<double> getDouble(int col);

    ~SQLiteResult();
};
