/*
 * Copyright (c) 2017, 2022, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef MYSQLSHDK_LIBS_DB_SESSION_H_
#define MYSQLSHDK_LIBS_DB_SESSION_H_

#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#ifdef _WIN32
#include <windows.h>
#ifdef WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#endif
using socket_t = SOCKET;
#else
using socket_t = int;
#endif /* _WIN32 */

#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/result.h"
#include "mysqlshdk/libs/db/ssl_options.h"
#include "mysqlshdk/libs/utils/error.h"
#include "mysqlshdk/libs/utils/log_sql.h"
#include "mysqlshdk/libs/utils/utils_sqlstring.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/version.h"
#include "mysqlshdk_export.h"

namespace mysqlshdk {
namespace db {

inline bool is_mysql_client_error(int code) {
  return code >= 2000 && code <= 2999;
}

inline bool is_mysql_server_error(int code) {
  return !is_mysql_client_error(code) && code > 0 && code < 50000;
}

class Error : public shcore::Error {
 public:
  Error() : Error("", 0, nullptr) {}

  Error(const char *what, int code) : Error(what, code, nullptr) {}

  Error(const std::string &what, int code) : shcore::Error(what, code) {}

  Error(const char *what, int code, const char *sqlstate)
      : shcore::Error(what, code), sqlstate_(sqlstate ? sqlstate : "") {}

  const char *sqlstate() const { return sqlstate_.c_str(); }

  std::string format() const override {
    if (sqlstate_.empty())
      return "MySQL Error " + std::to_string(code()) + ": " + what();
    else
      return "MySQL Error " + std::to_string(code()) + " (" + sqlstate() +
             "): " + what();
  }

 private:
  std::string sqlstate_;
};

class SHCORE_PUBLIC ISession {
 public:
  // Connection
  virtual void connect(const std::string &uri) {
    connect(mysqlshdk::db::Connection_options(uri));
  }

  void connect(const mysqlshdk::db::Connection_options &data);

  virtual uint64_t get_connection_id() const = 0;

  virtual socket_t get_socket_fd() const = 0;

  virtual const mysqlshdk::db::Connection_options &get_connection_options()
      const = 0;

  virtual const char *get_ssl_cipher() const = 0;

  virtual mysqlshdk::utils::Version get_server_version() const = 0;

  // Execution
  virtual std::shared_ptr<IResult> querys(const char *sql, size_t len,
                                          bool buffered = false) = 0;

  inline std::shared_ptr<IResult> query(std::string_view sql,
                                        bool buffered = false) {
    return querys(sql.data(), sql.length(), buffered);
  }

  /**
   * Executes a query with a user-defined function (UDF)
   *
   * Due to how UDFs are implemented in the server, they behave differently than
   * non UDFs, which means a UDF aware query method must used when dealing with
   * them.
   *
   * @param sql The SQL query with an UDF to execute
   * @param buffered True if the results should be buffered (read all at once),
   * false if they should be read row-by-row
   * @return std::shared_ptr<IResult> the result set of the query
   */
  virtual std::shared_ptr<IResult> query_udf(std::string_view sql,
                                             bool buffered = false) = 0;

  virtual void executes(const char *sql, size_t len) = 0;

  inline void execute(std::string_view sql) {
    executes(sql.data(), sql.length());
  }

  /**
   * Execute query and perform client-side placeholder substitution
   * @param  sql  query with placeholders
   * @param  args values for placeholders
   * @return      query result
   *
   * @example
   * auto result = session->queryf("SELECT * FROM tbl WHERE id = ?", my_id);
   */
  template <typename... Args>
  inline std::shared_ptr<IResult> queryf(const std::string &sql,
                                         const Args &... args) {
    return query(shcore::sqlformat(sql, args...));
  }

  template <typename... Args>
  inline void executef(const std::string &sql, const Args &... args) {
    execute(shcore::sqlformat(sql, args...));
  }

  // Disconnection
  void close();

  virtual bool is_open() const = 0;

  virtual const Error *get_last_error() const = 0;

  virtual ~ISession() = default;

  virtual std::string escape_string(const std::string &s) const = 0;
  virtual std::string escape_string(const char *buffer, size_t len) const = 0;

  const char *get_sql_mode() {
    if (!m_sql_mode && is_open()) refresh_sql_mode();
    return m_sql_mode ? m_sql_mode->c_str() : "";
  }

  bool ansi_quotes_enabled() {
    if (!m_sql_mode && is_open()) refresh_sql_mode();
    return m_ansi_quotes_enabled;
  }

  void refresh_sql_mode() {
    assert(is_open());
    try {
      auto result = query("select @@sql_mode;");
      auto row = result->fetch_one();

      if (row && !row->is_null(0)) {
        const auto sql_mode = shcore::str_upper(row->get_string(0));
        m_sql_mode = std::make_unique<std::string>(sql_mode);
        m_ansi_quotes_enabled =
            sql_mode.find("ANSI_QUOTES") != std::string::npos;
      } else {
        throw std::runtime_error("Missing sql_mode");
      }
    } catch (...) {
      m_sql_mode = std::make_unique<std::string>("");
      m_ansi_quotes_enabled = false;
    }
  }

 protected:
  virtual void do_connect(const mysqlshdk::db::Connection_options &data) = 0;
  virtual void do_close() = 0;

 private:
  std::unique_ptr<std::string> m_sql_mode;
  bool m_ansi_quotes_enabled = false;
};

}  // namespace db
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_DB_SESSION_H_
