

#ifndef HELPERS_SQLITEDATABASE_H
#define HELPERS_SQLITEDATABASE_H

#include "easylogging++.h"
#include <ctime>
#include <sqlite3.h>

// SQLiteDatabase class manages the lifecycle and operations of an SQLite
// database connection.
class SQLiteDatabase {
private:
  sqlite3 *db; // Pointer to the SQLite database
private:
  // Base case for bind_values. Does nothing and is used to handle the case
  // where no arguments are provided.
  void bind_values(sqlite3_stmt *) {}

  // Binds an integer value to the first placeholder in the prepared SQL
  // statement.
  // stmt: Pointer to the prepared SQL statement.
  // value: Integer value to bind.
  void bind_values(sqlite3_stmt *stmt, int value) {
    sqlite3_bind_int(stmt, 1, value);
  }

  // Binds a time_t value to the second placeholder in the prepared SQL
  // statement.
  // This is used for binding time values (typically representing dates or
  // timestamps).
  // stmt: Pointer to the prepared SQL statement.
  // value: time_t value to bind.
  void bind_values(sqlite3_stmt *stmt, const std::time_t &value) {
    sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(value));
  }

  // Binds a double value to the third placeholder in the prepared SQL
  // statement.
  // This is typically used for binding floating-point numbers.
  // stmt: Pointer to the prepared SQL statement.
  // value: Double value to bind.
  void bind_values(sqlite3_stmt *stmt, double value) {
    sqlite3_bind_double(stmt, 3, value);
  }

  // Template function for binding multiple values to a prepared SQL statement.
  // This function is variadic, allowing it to take multiple arguments of
  // different types. It recursively calls itself to bind each argument in the
  // parameter pack to the statement.
  // stmt: Pointer to the prepared SQL statement.
  // first: The first value in the parameter pack to bind.
  // rest...: The remaining values in the parameter pack.
  template <typename First, typename... Rest>
  void bind_values(sqlite3_stmt *stmt, First first, Rest... rest) {
    bind_values(stmt, first);
    bind_values(stmt, rest...);
  }

public:
  // Constructor: Opens a connection to the SQLite database at the specified
  // path.
  // Throws a runtime_error if the database cannot be opened.
  SQLiteDatabase(const std::string &path) {
    if (sqlite3_open_v2(path.c_str(), &db,
                        SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE,
                        nullptr) != SQLITE_OK) {

      LOG(ERROR) << "Error opening SQLite database: " << sqlite3_errmsg(db);
    }
  }

  // Destructor: Closes the database connection.
  ~SQLiteDatabase() {
    if (db) {
      sqlite3_close(db);
    }
  }

  // Executes a given SQL statement without expecting a return value.
  // Throws a runtime_error if the execution fails.
  void execute(const std::string &sql) {
    char *zErrMsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &zErrMsg) !=
        SQLITE_OK) {
      std::string error = "SQL error: " + std::string(zErrMsg);
      sqlite3_free(zErrMsg);
      /* throw std::runtime_error(error); */
      LOG(ERROR) << "SQL error: " << zErrMsg;
    }
  }

  // Prepares an SQL statement for execution and returns a pointer to the
  // prepared statement. Throws a runtime_error if the statement preparation
  // fails.
  sqlite3_stmt *prepare(const std::string &sql) {
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
      /* throw std::runtime_error("Error preparing statement: " + */
      /*                          std::string(sqlite3_errmsg(db))); */
      LOG(ERROR) << "Error preparing statement: " << sqlite3_errmsg(db);
    }
    return stmt;
  }

  // Inserts data into the database using a prepared statement with variable
  // arguments.
  // Returns the ID of the inserted row
  // Throws a runtime_error on execution failure.
  template <typename... Args>
  int insert_data(const std::string &query, Args... args) {
    sqlite3_stmt *stmt = prepare(query);
    bind_values(stmt, args...);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
      std::string error = "Error executing insert statement: " +
                          std::string(sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      LOG(ERROR) << error;
      throw std::runtime_error(error);
    }

    int returned_id = sqlite3_column_int(stmt, 0);

    // Expect the next step to be SQLITE_DONE
    if (sqlite3_step(stmt) != SQLITE_DONE) {
      std::string error =
          "Execution didn't finish: " + std::string(sqlite3_errmsg(db));
      sqlite3_finalize(stmt);
      LOG(ERROR) << error;
      throw std::runtime_error(error);
    }

    sqlite3_finalize(stmt);
    return returned_id;
  }
};

#endif
