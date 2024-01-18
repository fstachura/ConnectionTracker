#pragma once
#include <cstddef>
#include <iostream>
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

    template<typename T, typename... Args>
    SQLiteStatement& bindWithParam(int param, T arg, Args... args) {
        return this->bind(param, arg).bindWithParam(param+1, args...);
    }

    template<typename T>
    SQLiteStatement& bindWithParam(int param, T arg) {
        return this->bind(param, arg);
    }
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

    template<typename T, typename... Args>
    SQLiteStatement& bindMany(T arg, Args... args) {
        return bindWithParam(1, arg, args...);
    }
};

class SQLiteResult {
    std::shared_ptr<sqlite3_stmt> stmt;

    friend class SQLiteStatement; 
    SQLiteResult(std::shared_ptr<sqlite3_stmt> stmt);
    int checkSqliteErr(int result);
    bool checkNullOrInvalidType(int sqliteType, int col);
public:
    bool next();

    std::optional<std::string> getString(int col);
    std::optional<std::vector<unsigned char>> getBlob(int col);
    std::optional<int> getInt(int col);
    std::optional<long> getLong(int col);
    std::optional<double> getDouble(int col);

    ~SQLiteResult();
};

template<typename... T>
class SQLiteTypedResult {
    SQLiteResult result;

    template<typename Q, typename... Args> 
    std::tuple<std::optional<Q>, std::optional<Args>...> get(int col) {
        return std::tuple_cat(get<Q>(col), get<Args...>(col+1));
    }

    template<>
    std::tuple<std::optional<std::string>> get(int col) {
        return result.getString(col);
    }

    template<>
    std::tuple<std::optional<std::vector<unsigned char>>> get(int col) {
        return result.getBlob(col);
    }

    template<>
    std::tuple<std::optional<int>> get(int col) {
        return result.getInt(col);
    }

    template<>
    std::tuple<std::optional<long>> get(int col) {
        return result.getLong(col);
    }

    template<>
    std::tuple<std::optional<double>> get(int col) {
        return result.getDouble(col);
    }

public:
    SQLiteTypedResult(SQLiteResult arg): result(arg) {}

    bool next() {
        return result.next();
    }

    std::tuple<std::optional<T>...> get() {
        return get<T...>(0);
    }
};
