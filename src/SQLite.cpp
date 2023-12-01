#include "SQLite.hpp"
#include <iostream>
#include <vector>

SQLiteException::SQLiteException(int errcode, std::string msg): errcode(errcode), msg(msg) { }
SQLiteException::SQLiteException(std::string msg): errcode(0), msg(msg) { }

const char* SQLiteException::what() const noexcept {
    return msg.c_str();
}

int SQLiteException::getErrcode() const noexcept {
    return errcode;
}


SQLite::SQLite(std::string path) {
    sqlite3* tmp_db;
    int result = sqlite3_open(path.c_str(), &tmp_db);
    db.reset(tmp_db, sqlite3_close);
    checkSqliteErr(result);
}

int SQLite::checkSqliteErr(int result) {
    if(result != SQLITE_OK && result != SQLITE_DONE) {
        throw SQLiteException(result, sqlite3_errmsg(db.get()));
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

std::pair<SQLiteStatement, std::string_view> SQLite::prepareOne(std::string_view view) {
    sqlite3_stmt* sql_stmt;
    const char* tail;
    checkSqliteErr(sqlite3_prepare_v2(db.get(), view.data(), view.size(), &sql_stmt, &tail));
    std::shared_ptr<sqlite3_stmt> ptr;
    ptr.reset(sql_stmt, sqlite3_finalize);
    return std::make_pair(SQLiteStatement(db, ptr), 
            view.substr(tail-view.data(), view.size()-(tail-view.data())));
}

std::vector<SQLiteStatement> SQLite::prepare(std::string str_stmt) {
    std::vector<SQLiteStatement> result;
    std::string_view view = str_stmt;
    while(true) {
        auto stmt = prepareOne(view);
        result.push_back(stmt.first);
        view = stmt.second;
        if(view.empty()) {
            break;
        }
    }
    return result;
}

// SQLiteStatement

SQLiteStatement::SQLiteStatement(std::shared_ptr<sqlite3> db, std::shared_ptr<sqlite3_stmt> stmt): 
    db(db), stmt(stmt) { }

void SQLiteStatement::checkIfBusy() {
    if(gettingResults && sqlite3_stmt_busy(stmt.get())) {
        throw SQLiteException("sqlite statement is still busy. make sure that all related instances of SQLiteResult were destroyed.");
    } else {
        gettingResults = false;
    }
}

SQLiteStatement& SQLiteStatement::bind(int param, const std::vector<unsigned char>& data) {
    checkIfBusy();
    checkSqliteErr(sqlite3_bind_blob64(
        stmt.get(), 
        param,
        data.data(),
        data.size(),
        nullptr
    ));
    return *this;
}

SQLiteStatement& SQLiteStatement::bind(int param, const std::string_view data) {
    checkIfBusy();
    checkSqliteErr(sqlite3_bind_text64(
        stmt.get(), 
        param,
        data.data(),
        data.size(),
        nullptr,
        SQLITE_UTF8
    ));
    return *this;
}

SQLiteStatement& SQLiteStatement::bind(int param, int value) {
    checkIfBusy();
    checkSqliteErr(sqlite3_bind_int(
        stmt.get(), 
        param,
        value  
    ));
    return *this;
}

SQLiteStatement& SQLiteStatement::bind(int param, long value) {
    checkIfBusy();
    checkSqliteErr(sqlite3_bind_int64(
        stmt.get(), 
        param,
        value  
    ));
    return *this;
}

SQLiteStatement& SQLiteStatement::bind(int param, double value) {
    checkIfBusy();
    checkSqliteErr(sqlite3_bind_double(
        stmt.get(), 
        param,
        value
    ));
    return *this;
}

SQLiteStatement& SQLiteStatement::bind(int param, std::nullptr_t) {
    checkIfBusy();
    checkSqliteErr(sqlite3_bind_null(
        stmt.get(), 
        param
    ));
    return *this;
}

int SQLiteStatement::checkSqliteErr(int result) {
    if(result != SQLITE_OK && result != SQLITE_DONE) {
        throw SQLiteException(result, sqlite3_errmsg(db.get()));
    }
    return result;
}

void SQLiteStatement::exec() {
    while(true) {
        int result = sqlite3_step(stmt.get());
        if(result == SQLITE_OK || result == SQLITE_DONE || result == SQLITE_EMPTY) {
            gettingResults = false;
            checkSqliteErr(sqlite3_reset(stmt.get()));
            return;
        }
        if(result != SQLITE_ROW) {
            checkSqliteErr(result);
        }
    }
}

SQLiteResult SQLiteStatement::getResults() {
    gettingResults = true;
    return SQLiteResult(stmt);
}

// SQLiteResult

SQLiteResult::SQLiteResult(std::shared_ptr<sqlite3_stmt> stmt): stmt(stmt) {}

bool SQLiteResult::next() {
    int result = sqlite3_step(stmt.get());
    if(result == SQLITE_DONE || result == SQLITE_EMPTY) {
        return false;
    } else if(result == SQLITE_ROW) {
        return true;
    }
    checkSqliteErr(result);
    return false;
}

std::optional<std::string> SQLiteResult::getString(int col) { 
    auto type = sqlite3_column_type(stmt.get(), col);
    checkSqliteErr(type);
    if(type == SQLITE_NULL) {
        return std::optional<std::string>();
    } else if(type != SQLITE_TEXT) {
        throw SQLiteException("invalid type " + std::to_string(type) + " for column " + std::to_string(col));
    }

    const char* data = (const char*)sqlite3_column_text(stmt.get(), col);
    if(data == nullptr) {
        checkSqliteErr(sqlite3_errcode(sqlite3_db_handle(stmt.get())));
        throw SQLiteException("sqlite_column_text returned null but type was not null " + std::to_string(type));
    }
    int len = sqlite3_column_bytes(stmt.get(), col);
    std::string result(data, len); 
    return std::optional(result);
}

std::optional<std::vector<unsigned char>> SQLiteResult::getBlob(int col) {
    auto type = sqlite3_column_type(stmt.get(), col);
    checkSqliteErr(type);
    if(type == SQLITE_NULL) {
        return std::optional<std::vector<unsigned char>>();
    } else if(type != SQLITE_BLOB) {
        throw SQLiteException("invalid type " + std::to_string(type) + " for column " + std::to_string(col));
    }

    const char* data = (const char*)sqlite3_column_blob(stmt.get(), col);
    if(data == nullptr) {
        checkSqliteErr(sqlite3_errcode(sqlite3_db_handle(stmt.get())));
        throw SQLiteException("sqlite_column_blob returned null but type was not null " + std::to_string(type));
    }
    int len = sqlite3_column_bytes(stmt.get(), col);
    std::vector<unsigned char> result(data, data+len); 
    return std::optional(result);
}

std::optional<int> SQLiteResult::getInt(int col) {
    auto type = sqlite3_column_type(stmt.get(), col);
    if(type == SQLITE_NULL) {
        return std::optional<int>();
    } else if(type != SQLITE_INTEGER) {
        throw SQLiteException("invalid type " + std::to_string(type) + " for column " + std::to_string(col));
    }

    return sqlite3_column_int(stmt.get(), col);
}

std::optional<long> SQLiteResult::getLong(int col) {
    auto type = sqlite3_column_type(stmt.get(), col);
    if(type == SQLITE_NULL) {
        return std::optional<long>();
    } else if(type != SQLITE_INTEGER) {
        throw SQLiteException("invalid type " + std::to_string(type) + " for column " + std::to_string(col));
    }

    return sqlite3_column_int64(stmt.get(), col);
}

std::optional<double> SQLiteResult::getDouble(int col) {
    auto type = sqlite3_column_type(stmt.get(), col);
    if(type == SQLITE_NULL) {
        return std::optional<double>();
    } else if(type != SQLITE_FLOAT) {
        throw SQLiteException("invalid type " + std::to_string(type) + " for column " + std::to_string(col));
    }

    return sqlite3_column_double(stmt.get(), col);
}

int SQLiteResult::checkSqliteErr(int result) {
    if(result != SQLITE_OK && result != SQLITE_DONE) {
        throw SQLiteException(result, sqlite3_errmsg(sqlite3_db_handle(stmt.get())));
    }
    return result;
}

SQLiteResult::~SQLiteResult() {
    checkSqliteErr(sqlite3_reset(stmt.get()));
}
