/* Copyright (c) 2017, 2021, Oracle and/or its affiliates.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License, version 2.0,
 as published by the Free Software Foundation.

 This program is also distributed with certain software (including
 but not limited to OpenSSL) that is licensed under separate terms, as
 designated in a particular file or component or in included license
 documentation.  The authors of MySQL hereby grant you an additional
 permission to link the program and your derivative works with the
 separately licensed software that they have included with MySQL.
 This program is distributed in the hope that it will be useful,  but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 the GNU General Public License, version 2.0, for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#include <string>
#include "mysqlshdk/libs/db/mysql/session.h"
#include "unittest/test_utils/command_line_test.h"
#include "utils/utils_file.h"
#include "utils/utils_path.h"

#ifndef MAX_PATH
const int MAX_PATH = 4096;
#endif

extern mysqlshdk::utils::Version g_target_server_version;

namespace tests {

TEST_F(Command_line_test, bug24912358) {
  // Tests with X Protocol Session
  {
    std::string uri = "--uri=" + _uri;
    execute({_mysqlsh, uri.c_str(), "--sql", "-e", "select -127 << 1.1", NULL});
    MY_EXPECT_MULTILINE_OUTPUT(
        "select -127 << 1.1",
        multiline({"-127 << 1.1", "18446744073709551362"}), _output);
  }

  {
    std::string uri = "--uri=" + _uri;
    execute(
        {_mysqlsh, uri.c_str(), "--sql", "-e", "select -127 << -1.1", NULL});
    MY_EXPECT_MULTILINE_OUTPUT("select -127 << 1.1",
                               multiline({"-127 << -1.1", "0"}), _output);
  }

  // Tests with Classic Session
  {
    std::string uri = "--uri=" + _mysql_uri;
    execute({_mysqlsh, uri.c_str(), "--sql", "-e", "select -127 << 1.1", NULL});
    MY_EXPECT_MULTILINE_OUTPUT(
        "select -127 << 1.1",
        multiline({"-127 << 1.1", "18446744073709551362"}), _output);
  }

  {
    std::string uri = "--uri=" + _mysql_uri;
    execute(
        {_mysqlsh, uri.c_str(), "--sql", "-e", "select -127 << -1.1", NULL});
    MY_EXPECT_MULTILINE_OUTPUT("select -127 << 1.1",
                               multiline({"-127 << -1.1", "0"}), _output);
  }
}

#ifdef HAVE_V8
TEST_F(Command_line_test, bug23508428) {
  // Test if the xplugin is installed using enableXProtocol in the --dba option
  // In 8.0.4, the mysqlx_cache_cleaner is also supposed to be installed
  // In 8.0.11+, both plugins are built-in, cannot be uninstalled
  std::string uri = "--uri=" + _mysql_uri;

  execute({_mysqlsh, uri.c_str(), "--sqlc", "-e", "uninstall plugin mysqlx;",
           NULL});
  if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 5)) {
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "ERROR: 1619 (HY000) at line 1: Built-in plugins "
        "cannot be deleted");
  }

  if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 4)) {
    execute({_mysqlsh, uri.c_str(), "--sqlc", "-e",
             "uninstall plugin mysqlx_cache_cleaner;", NULL});
    if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 5)) {
      MY_EXPECT_CMD_OUTPUT_CONTAINS(
          "ERROR: 1619 (HY000) at line 1: Built-in plugins "
          "cannot be deleted");
    }
  }

  if (_target_server_version < mysqlshdk::utils::Version(8, 0, 5)) {
    execute(
        {_mysqlsh, uri.c_str(), "--mysql", "--dba", "enableXProtocol", NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "enableXProtocol: Installing plugin "
        "mysqlx...");
    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "enableXProtocol: successfully installed the X protocol plugin!");
  }

  execute({_mysqlsh, uri.c_str(), "--interactive=full", "-e",
           "session.runSql('SELECT COUNT(*) FROM information_schema.plugins "
           "WHERE PLUGIN_NAME in (\"mysqlx\", \"mysqlx_cache_cleaner\")')."
           "fetchOne()",
           NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS("[");
  if (_target_server_version >= mysqlshdk::utils::Version(8, 0, 4)) {
    MY_EXPECT_CMD_OUTPUT_CONTAINS("    2");
  } else {
    MY_EXPECT_CMD_OUTPUT_CONTAINS("    1");
  }
  MY_EXPECT_CMD_OUTPUT_CONTAINS("]");

  execute({_mysqlsh, uri.c_str(), "--mysql", "--dba", "enableXProtocol", NULL});
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "enableXProtocol: The X Protocol plugin is already enabled and listening "
      "for connections on port " +
      _port);
}
#endif

TEST_F(Command_line_test, bug24905066) {
  // Tests URI formatting using classic protocol
  {
#ifdef _WIN32
    execute({_mysqlsh, "--mysql", "-i", "--uri",
             "root:@\\\\.\\(d:\\path\\to\\whatever\\socket.sock)", NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Creating a Classic session to "
        "'root@\\\\.\\d%3A%5Cpath%5Cto%5Cwhatever%5Csocket.sock'");
#else   // !_WIN32
    execute({_mysqlsh, "--mysql", "-i", "--uri",
             "root:@(/path/to/whatever/socket.sock)", NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Creating a Classic session to "
        "'root@/path%2Fto%2Fwhatever%2Fsocket.sock'");
#endif  // !_WIN32
  }

#ifndef _WIN32
  // Tests URI formatting using X protocol
  {
    execute({_mysqlsh, "--mysqlx", "-i", "--uri",
             "root:@(/path/to/whatever/socket.sock)", "-e", "1", NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "Creating an X protocol session to "
        "'root@/path%2Fto%2Fwhatever%2Fsocket.sock'");
  }
#endif  // !_WIN32

  // Tests the connection fails if invalid schema is provided on classic session
  {
    std::string uri = _mysql_uri + "/some_unexisting_schema";

    execute({_mysqlsh, "--mysql", "-i", "--uri", uri.c_str(), NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS(
        "MySQL Error 1049 (42000): Unknown database "
        "'some_unexisting_schema'");
  }

  // Tests the connection fails if invalid schema is provided on x session
  {
    std::string uri = _uri + "/some_unexisting_schema";

    execute(
        {_mysqlsh, "--mysqlx", "-i", "--uri", uri.c_str(), "-e", "1", NULL});

    MY_EXPECT_CMD_OUTPUT_CONTAINS("Unknown database 'some_unexisting_schema'");
  }
}

TEST_F(Command_line_test, bug24967838) {
  // Check if processor is available
  ASSERT_NE(system(NULL), 0);

  // Tests that if error happens when executing script, return code should be
  // different then 0
  {
    char cmd[MAX_PATH];
    std::snprintf(cmd, MAX_PATH,
#ifdef _WIN32
                  "echo no error | %s --uri=%s --sql --database=mysql "
                  "2> nul",
#else
                  "echo \"no error\" | %s --uri=%s --sql --database=mysql "
                  "2> /dev/null",
#endif
                  _mysqlsh, _uri.c_str());

    EXPECT_NE(system(cmd), 0);
  }

  // Test that 0 (130 on Windows) is returned of successful run of the command
  {
    char cmd[MAX_PATH];
    std::snprintf(cmd, MAX_PATH,
#ifdef _WIN32
                  "echo DROP TABLE IF EXISTS test; | %s --uri=%s "
#else
                  "echo \"DROP TABLE IF EXISTS test;\" | %s --uri=%s "
#endif
                  "--sql --database=mysql",
                  _mysqlsh, _uri.c_str());

    EXPECT_EQ(system(cmd), 0);
  }
}

TEST_F(Command_line_test, Bug25974014) {
  // Check if processor is available
  ASSERT_NE(system(NULL), 0);

  char buf[MAX_PATH];
  char cmd[MAX_PATH];
  std::string out;
  FILE *fp;

  // X session
  std::snprintf(cmd, MAX_PATH,
#ifdef _WIN32
                "echo kill CONNECTION_ID(); use mysql; | %s --uri=%s --sql "
                "--interactive 2> nul",
#else
                "echo \"kill CONNECTION_ID(); use mysql;\" | %s --uri=%s "
                "--sql --interactive 2> /dev/null",
#endif
                _mysqlsh, _uri.c_str());

#ifdef _WIN32
  fp = _popen(cmd, "r");
#else
  fp = popen(cmd, "r");
#endif
  ASSERT_NE(nullptr, fp);
  while (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
    out.append(buf);
  }
  EXPECT_NE(std::string::npos, out.find("Attempting to reconnect"));

  // Classic session
  out.clear();
  std::snprintf(cmd, MAX_PATH,
#ifdef _WIN32
                "echo kill CONNECTION_ID(); use mysql; | %s --uri=%s --sql "
                "--interactive 2> nul",
#else
                "echo \"kill CONNECTION_ID(); use mysql;\" | %s --uri=%s "
                "--sql --interactive 2> /dev/null",
#endif
                _mysqlsh, _mysql_uri.c_str());

#ifdef _WIN32
  fp = _popen(cmd, "r");
#else
  fp = popen(cmd, "r");
#endif
  ASSERT_NE(nullptr, fp);
  while (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
    out.append(buf);
  }
  EXPECT_NE(std::string::npos, out.find("Attempting to reconnect"));
}

TEST_F(Command_line_test, Bug25105307) {
  execute({_mysqlsh, _uri.c_str(), "--sql", "-e",
           "kill CONNECTION_ID(); use mysql;", NULL});

  MY_EXPECT_CMD_OUTPUT_CONTAINS("interrupted");
  MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS("Attempting to reconnect");
}

TEST_F(Command_line_test, retain_schema_after_reconnect) {
  // Check if processor is available
  ASSERT_NE(system(NULL), 0);

  char cmd[MAX_PATH];
  std::snprintf(
      cmd, MAX_PATH,
#ifdef _WIN32
      "echo kill CONNECTION_ID(); show tables; | %s --uri=%s/mysql --sql "
      "--interactive 2> nul",
#else
      "echo \"use mysql;\nkill CONNECTION_ID(); show tables;\" | %s --uri=%s "
      "--sql --interactive 2> /dev/null",
#endif
      _mysqlsh, _uri.c_str());

#ifdef _WIN32
  FILE *fp = _popen(cmd, "r");
#else
  FILE *fp = popen(cmd, "r");
#endif
  ASSERT_NE(nullptr, fp);
  char buf[MAX_PATH];
  /* Read the output a line at a time - output it. */
  std::string out;
  while (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
    out.append(buf);
  }
  EXPECT_NE(std::string::npos, out.find("Attempting to reconnect"));
  EXPECT_NE(std::string::npos, out.find("/mysql'.."));
}

TEST_F(Command_line_test, duplicate_not_connected_error) {
  // executing multiple statements while not connected should print
  // Not Connected just once, not once for each statement
  execute({_mysqlsh, "--sql", "-e", "select 1; select 2; select 3;", NULL});

  EXPECT_EQ("ERROR: Not connected.\n", _output);
}

TEST_F(Command_line_test, bug25653170) {
  // Executing scripts with incomplete SQL silently fails
  int rc =
      execute({_mysqlsh, _uri.c_str(), "--sql", "-e", "select 1 /*", NULL});
  EXPECT_EQ(1, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "ERROR: 1064 at line 1: You have an error in your SQL syntax; check the "
      "manual "
      "that corresponds to your MySQL server version for the right syntax to "
      "use near '/*' at line 1");
}

// This test used to be for ensuring socket connections are rejected for
// InnoDB cluster, but they are now allowed.
TEST_F(Command_line_test, bug26970629) {
  const std::string variable = "socket";
  const std::string host =
      "--host="
#ifdef _WIN32
      "."
#else   // !_WIN32
      "localhost"
#endif  // !_WIN32
      ;
  const std::string pwd = "--password=" + _pwd;
  std::string socket;

  mysqlshdk::db::Connection_options options;
  options.set_host(_host);
  options.set_port(std::stoi(_mysql_port));
  options.set_user(_user);
  options.set_password(_pwd);

  auto session = mysqlshdk::db::mysql::Session::create();
  session->connect(options);

  auto result = session->query("show variables like '" + variable + "'");
  auto row = result->fetch_one();

  if (row) {
    std::string socket_path = row->get_as_string(1);

#ifdef _WIN32
    socket = "--socket=" + socket_path;
#else   // !_WIN32
    if (shcore::path_exists(socket_path)) {
      socket = "--socket=" + socket_path;
    } else {
      result = session->query("show variables like 'datadir'");
      row = result->fetch_one();
      socket_path = shcore::path::normalize(
          shcore::path::join_path(row->get_as_string(1), socket_path));

      if (shcore::path_exists(socket_path)) {
        socket = "--socket=" + socket_path;
      }
    }
#endif  // !_WIN32
  }

#ifdef _WIN32
  result = session->query("show variables like 'named_pipe'");
  row = result->fetch_one();

  if (!row || row->get_as_string(1) != "ON") {
    socket.clear();
  }
#endif  // _WIN32

  session->close();

  if (socket.empty()) {
    SCOPED_TRACE("Socket/Pipe Connections are Disabled, they must be enabled.");
    FAIL();
  } else {
    std::string usr = "--user=" + _user;

#ifdef HAVE_V8
    execute({_mysqlsh, "--js", usr.c_str(), pwd.c_str(), host.c_str(),
             socket.c_str(), "-e", "dba.createCluster('sample')", NULL});
#else
    execute({_mysqlsh, "--py", usr.c_str(), pwd.c_str(), host.c_str(),
             socket.c_str(), "-e", "dba.create_cluster('sample')", NULL});
#endif
    SCOPED_TRACE(socket.c_str());
    MY_EXPECT_CMD_OUTPUT_NOT_CONTAINS(
        "a MySQL session through TCP/IP is required to perform this operation");
  }
}

#ifdef HAVE_V8
TEST_F(Command_line_test, bug28814112_js) {
  // SEG-FAULT WHEN CALLING SHELL.SETCURRENTSCHEMA() WITHOUT AN ACTIVE SHELL
  // SESSION
  int rc = execute({_mysqlsh, "-e", "shell.setCurrentSchema('mysql')", NULL});
  EXPECT_EQ(1, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "Shell.setCurrentSchema: An open session is required to perform this "
      "operation. "
      "(RuntimeError)");
}
#else
TEST_F(Command_line_test, bug28814112_py) {
  // SEG-FAULT WHEN CALLING SHELL.SETCURRENTSCHEMA() WITHOUT AN ACTIVE SHELL
  // SESSION
  int rc = execute({_mysqlsh, "-e", "shell.set_current_schema('mysql')", NULL});
  EXPECT_EQ(1, rc);
  MY_EXPECT_CMD_OUTPUT_CONTAINS(
      "Shell.set_current_schema: An open session is required to perform this "
      "operation.");
}
#endif

TEST_F(Command_line_test, batch_ansi_quotes) {
  // Check if processor is available
  ASSERT_NE(system(NULL), 0);
  char cmd[MAX_PATH];
  std::snprintf(
      cmd, MAX_PATH,
#ifdef _WIN32
      R"(echo set sql_mode = 'ANSI_QUOTES'; select "\"";select version();#";)"
#else
      R"(echo "set sql_mode = 'ANSI_QUOTES'; select \"\\\"\";select version();#\"")"
#endif
      " | %s --uri=%s --sql 2>&1",
      _mysqlsh, _uri.c_str());

#ifdef _WIN32
  FILE *fp = _popen(cmd, "r");
#else
  FILE *fp = popen(cmd, "r");
#endif
  ASSERT_NE(nullptr, fp);
  char buf[MAX_PATH];
  /* Read the output a line at a time - output it. */
  std::string out;
  while (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
    out.append(buf);
  }
  EXPECT_NE(std::string::npos,
            out.find(R"(Unknown column '\";select version();#')"));
}

}  // namespace tests
