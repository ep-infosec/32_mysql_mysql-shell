/*
 * Copyright (c) 2014, 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_GENERAL_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_GENERAL_H_

#include <functional>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#undef DELETE
#undef ERROR
#else
#include <unistd.h>
#endif

#include "mysqlshdk/include/scripting/naming_style.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/include/shellcore/ishell_core.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/ssh/ssh_connection_options.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {

/**
 * Calculate in compile time length/size of an array.
 *
 * @param array
 * @return Return size of an array.
 */
template <class T, std::size_t N>
constexpr static std::size_t array_size(const T (&)[N]) noexcept {
  return N;
}

class Scoped_callback {
 public:
  explicit Scoped_callback(const std::function<void()> &c) : callback(c) {}

  ~Scoped_callback();

  void call() {
    if (!cancelled && !called) {
      callback();
      called = true;
    }
  }

  void cancel() { cancelled = true; }

  const std::exception_ptr &exception() const { return exception_ptr; }

  void check() {
    if (exception_ptr) std::rethrow_exception(exception_ptr);
  }

 private:
  std::function<void()> callback;
  std::exception_ptr exception_ptr;
  bool cancelled = false;
  bool called = false;
};

class Scoped_callback_list {
 public:
  ~Scoped_callback_list() {
    try {
      call();
    } catch (...) {
      exception_ptr = std::current_exception();
    }
  }

  void push_back(const std::function<void()> &c) { callbacks.push_back(c); }

  void push_front(const std::function<void()> &c) { callbacks.push_front(c); }

  void call() {
    if (!cancelled && !called) {
      called = true;
      for (const auto &cb : callbacks) {
        cb();
      }
    }
  }

  void cancel() { cancelled = true; }

  const std::exception_ptr &exception() const { return exception_ptr; }

  void check() {
    if (exception_ptr) std::rethrow_exception(exception_ptr);
  }

 private:
  std::list<std::function<void()>> callbacks;
  std::exception_ptr exception_ptr;
  bool cancelled = false;
  bool called = false;
};

using on_leave_scope = Scoped_callback;

enum class OperatingSystem {
  UNKNOWN,
  DEBIAN,
  REDHAT,
  LINUX,
  WINDOWS,
  MACOS,
  SOLARIS
};
std::string SHCORE_PUBLIC to_string(OperatingSystem os_type);

bool SHCORE_PUBLIC is_valid_identifier(const std::string &name);
mysqlshdk::db::Connection_options SHCORE_PUBLIC
get_connection_options(const std::string &uri, bool set_defaults = true);
mysqlshdk::ssh::Ssh_connection_options SHCORE_PUBLIC
get_ssh_connection_options(const std::string &uri, bool set_defaults = true,
                           const std::string &config_path = "");
void SHCORE_PUBLIC update_connection_data(
    mysqlshdk::db::Connection_options *connection_options,
    const std::string &user, const char *password, const std::string &host,
    int port, const mysqlshdk::utils::nullable<std::string> &sock,
    const std::string &database, const mysqlshdk::db::Ssl_options &ssl_options,
    const std::string &auth_method, bool get_server_public_key,
    const std::string &server_public_key_path,
    const std::string &connect_timeout, const std::string &compression,
    const std::string &compress_algorithm,
    mysqlshdk::utils::nullable<int64_t> compress_level);

std::string SHCORE_PUBLIC get_system_user();

std::string SHCORE_PUBLIC strip_password(const std::string &connstring);

std::string SHCORE_PUBLIC strip_ssl_args(const std::string &connstring);

char SHCORE_PUBLIC *mysh_get_stdin_password(const char *prompt);

std::vector<std::string> SHCORE_PUBLIC
split_string(const std::string &input, const std::string &separator,
             bool compress = false);
std::vector<std::string> SHCORE_PUBLIC
split_string_chars(const std::string &input, const std::string &separator_chars,
                   bool compress = false);

bool SHCORE_PUBLIC match_glob(const std::string_view pattern,
                              const std::string_view s,
                              bool case_sensitive = false);

std::string SHCORE_PUBLIC to_camel_case(const std::string &name);
std::string SHCORE_PUBLIC from_camel_case(const std::string &name);
std::string SHCORE_PUBLIC from_camel_case_to_dashes(const std::string &name);

std::string SHCORE_PUBLIC errno_to_string(int err);

struct Account {
  enum class Auto_quote {
    /**
     * No auto-quotes, string must be a valid account name.
     */
    NO,
    /**
     * Host will be auto-quoted, multiple unqouted '@' characters are NOT
     * allowed.
     */
    HOST,
    /**
     * String is a result of i.e. CURRENT_USER() function and is not quoted at
     * all. Host will be auto-quoted, multiple unqouted '@' characters are
     * allowed, the last one marks the beginning of the host name.
     */
    USER_AND_HOST,
  };

  std::string user;
  std::string host;

  bool operator<(const Account &a) const {
    return std::tie(user, host) < std::tie(a.user, a.host);
  }

  bool operator==(const Account &a) const {
    return user == a.user && host == a.host;
  }
};

void SHCORE_PUBLIC split_account(
    const std::string &account, std::string *out_user, std::string *out_host,
    Account::Auto_quote auto_quote = Account::Auto_quote::NO);
Account SHCORE_PUBLIC
split_account(const std::string &account,
              Account::Auto_quote auto_quote = Account::Auto_quote::NO);

template <typename C>
std::vector<Account> to_accounts(
    const C &c, Account::Auto_quote auto_quote = Account::Auto_quote::NO) {
  std::vector<Account> result;

  for (const auto &i : c) {
    result.emplace_back(split_account(i, auto_quote));
  }

  return result;
}

std::string SHCORE_PUBLIC make_account(const std::string &user,
                                       const std::string &host);
std::string SHCORE_PUBLIC make_account(const Account &account);

std::string SHCORE_PUBLIC get_member_name(const std::string &name,
                                          shcore::NamingStyle style);

/**
 * Ensures at most 2 identifiers are found on the string:
 *  - if 2 identifiers are found then they are set to schema and table
 *  - If 1 identifier is found it is set to table
 * Ensures the table name is not empty.
 */
void SHCORE_PUBLIC split_schema_and_table(const std::string &str,
                                          std::string *out_schema,
                                          std::string *out_table,
                                          bool allow_ansi_quotes = false);

/**
 * Ensures at most 3 identifiers are found on the string:
 *  - if 3 identifiers are found then they are set to schema, table and object
 *  - if 2 identifiers are found then they are set to table and object
 *  - if 1 identifier is found it is set to object
 * Ensures the object name is not empty.
 */
void SHCORE_PUBLIC split_schema_table_and_object(
    const std::string &str, std::string *out_schema, std::string *out_table,
    std::string *out_object, bool allow_ansi_quotes = false);

void SHCORE_PUBLIC split_priv_level(const std::string &str,
                                    std::string *out_schema,
                                    std::string *out_object,
                                    size_t *out_leftover = nullptr);

std::string SHCORE_PUBLIC unquote_identifier(const std::string &str,
                                             bool allow_ansi_quotes = false);

std::string SHCORE_PUBLIC unquote_sql_string(const std::string &str);

/** Substitute variables in string.
 *
 * str_subvar("hello ${foo}",
 *        [](const std::string&) { return "world"; },
 *        "${", "}");
 *    --> "hello world";
 *
 * str_subvar("hello $foo!",
 *        [](const std::string&) { return "world"; },
 *        "$", "");
 *    --> "hello world!";
 *
 * If var_end is "", then the variable name will span until the
 */
std::string SHCORE_PUBLIC str_subvars(
    const std::string &s,
    const std::function<std::string(const std::string &)> &subvar =
        [](const std::string &var) {
          return shcore::get_member_name(var, shcore::current_naming_style());
        },
    const std::string &var_begin = "<<<", const std::string &var_end = ">>>");

void SHCORE_PUBLIC sleep_ms(uint32_t ms);

OperatingSystem SHCORE_PUBLIC get_os_type();

/**
 * Provides host CPU type, i.e. aarch64, x86_64, etc.
 *
 * @return machine type
 */
std::string SHCORE_PUBLIC get_machine_type();

/**
 * Provides long version of mysqlsh, including version number, OS type, MySQL
 * version number and build type.
 *
 * @return version string
 */
const char *SHCORE_PUBLIC get_long_version();

#ifdef _WIN32

std::string SHCORE_PUBLIC last_error_to_string(DWORD code);

#endif  // _WIN32

template <class T>
T lexical_cast(const T &data) {
  return data;
}

template <class T, class S>
typename std::enable_if<std::is_same<T, std::string>::value, T>::type
lexical_cast(const S &data) {
  std::stringstream ss;
  if (std::is_same<S, bool>::value) ss << std::boolalpha;
  ss << data;
  return ss.str();
}

template <class T, class S>
typename std::enable_if<!std::is_same<T, std::string>::value, T>::type
lexical_cast(const S &data) {
  std::stringstream ss;
  ss << data;
  if (std::is_unsigned<T>::value && ss.peek() == '-')
    throw std::invalid_argument("Unable to perform conversion.");
  T t;
  ss >> t;
  if (ss.fail()) {
    if (std::is_same<T, bool>::value) {
      const auto &str = ss.str();
      if (shcore::str_caseeq(str, "true"))
        return true;
      else if (shcore::str_caseeq(str, "false"))
        return false;
    }
    throw std::invalid_argument("Unable to perform conversion.");
  } else if (!ss.eof()) {
    throw std::invalid_argument("Conversion did not consume whole input.");
  }
  return t;
}

template <class T, class S>
T lexical_cast(const S &data, T default_value) noexcept {
  try {
    return lexical_cast<T>(data);
  } catch (...) {
  }
  return default_value;
}

/**
 * Wrapper for the std::getline() function which removes the last character
 * if it's carriage return.
 */
std::istream &getline(std::istream &in, std::string &out);

/**
 * Verifies the status code of an application.
 *
 * @param status - status code to be checked
 * @param error - if execution was not successful, contains details on
 *                corresponding error
 *
 * @returns true if status code corresponds to a successful execution of an
 * application.
 */
bool verify_status_code(int status, std::string *error);

/**
 * Sets the environment variable 'name' to the value of 'value'.
 * If value is null or empty, variable is removed instead.
 *
 * @param name - name of the environment variable to set
 * @param value - value of the environment variable
 *
 * @returns true if the environment variable was set successfully.
 */
bool setenv(const char *name, const char *value);
bool setenv(const char *name, const std::string &value);
bool setenv(const std::string &name, const std::string &value);

/**
 * Sets the environment variable, string must be in form: name=value.
 * If value is empty, variable is removed instead.
 *
 * @param name_value - name and the new value of the environment variable
 *
 * @returns true if the environment variable was set successfully.
 */
bool setenv(const std::string &name_value);

/**
 * Clears the environment variable called 'name'.
 *
 * @param name - name of the environment variable to clear
 *
 * @returns true if the environment variable was cleared successfully.
 */
bool unsetenv(const char *name);
bool unsetenv(const std::string &name);

#ifdef _WIN32
#define STDIN_FILENO _fileno(stdin)
#define STDOUT_FILENO _fileno(stdout)
#define isatty _isatty
#endif

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_GENERAL_H_
