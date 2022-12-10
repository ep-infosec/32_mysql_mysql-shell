/*
 * Copyright (c) 2015, 2022, Oracle and/or its affiliates.
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

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include "modules/adminapi/common/sql.h"
#include "modules/mod_mysql_session.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/utils/utils_stacktrace.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "shell_script_tester.h"
#include "shellcore/base_session.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_path.h"

namespace shcore {

class Shell_js_dba_tests : public Shell_js_script_tester {
 protected:
  bool _have_ssl;
  std::string _sandbox_share;
  static bool have_sandboxes;

  // You can define per-test set-up and tear-down logic as usual.
  virtual void SetUp() {
    Shell_js_script_tester::SetUp();
    // All of the test cases share the same config folder
    // and setup script
    set_config_folder("js_devapi");
    set_setup_script("setup.js");
  }

  virtual void set_defaults() {
    Shell_js_script_tester::set_defaults();

    std::string user, host, password;
    auto connection_options = shcore::get_connection_options(_uri);

    if (connection_options.has_user()) user = connection_options.get_user();

    if (connection_options.has_host()) host = connection_options.get_host();

    if (connection_options.has_password())
      password = connection_options.get_password();

    std::string mysql_uri = "mysql://";
    std::string have_ssl;
    _have_ssl = true;

    assert(!m_port.empty());
    assert(!m_mysql_port.empty());

    std::string code = "var hostname = '" + hostname() + "';";
    exec_and_out_equals(code);
    code = "var real_hostname = '" + real_hostname() + "';";
    exec_and_out_equals(code);

    if (real_host_is_loopback())
      code = "var real_host_is_loopback = true;";
    else
      code = "var real_host_is_loopback = false;";
    exec_and_out_equals(code);

    code = "var hostname_ip = '" + hostname_ip() + "';";
    exec_and_out_equals(code);
    code = "var __user = '" + user + "';";
    exec_and_out_equals(code);
    code = "var __host = '" + host + "';";
    exec_and_out_equals(code);
    code = "var __port = " + m_port + ";";
    exec_and_out_equals(code);
    code = "var __schema = 'mysql';";
    exec_and_out_equals(code);
    code = "var __uri = '" + user + "@" + host + ":" + m_port + "';";
    exec_and_out_equals(code);
    code = "var __xhost_port = '" + host + ":" + m_port + "';";
    exec_and_out_equals(code);

    code = "var __host_port = '" + host + ":" + m_mysql_port + "';";
    exec_and_out_equals(code);
    code = "var __mysql_port = " + m_mysql_port + ";";
    exec_and_out_equals(code);
    for (int i = 0; i < tests::sandbox::k_num_ports; i++) {
      code = shcore::str_format("var __mysql_sandbox_port%i = %i;", i + 1,
                                _mysql_sandbox_ports[i]);
      exec_and_out_equals(code);

      // With 8.0.27, the GR Protocol Communication Stack became MySQL by
      // default and localAddress is set to the server port by default
      if (_target_server_version < mysqlshdk::utils::Version("8.0.27")) {
        code = shcore::str_format("var __mysql_sandbox_gr_port%i = %i;", i + 1,
                                  _mysql_sandbox_ports[i] * 10 + 1);
      } else {
        code = shcore::str_format("var __mysql_sandbox_gr_port%i = %i;", i + 1,
                                  _mysql_sandbox_ports[i]);
      }
      exec_and_out_equals(code);
      code = shcore::str_format(
          "var __sandbox_uri%i = 'mysql://root:root@localhost:%i';", i + 1,
          _mysql_sandbox_ports[i]);
      exec_and_out_equals(code);
      code = shcore::str_format(
          "var __hostname_uri%i = 'mysql://root:root@%s:%i';", i + 1,
          hostname().c_str(), _mysql_sandbox_ports[i]);
      exec_and_out_equals(code);
      code = shcore::str_format("var uri%i = 'localhost:%i';", i + 1,
                                _mysql_sandbox_ports[i]);
      exec_and_out_equals(code);
    }
    code = "var localhost = 'localhost'";
    exec_and_out_equals(code);
    code =
        "var add_instance_options = {HoSt:localhost, port: 0000, "
        "PassWord:'root', scheme:'mysql'};";
    exec_and_out_equals(code);

    code = "var add_instance_extra_opts = {};";
    exec_and_out_equals(code);

    _sandbox_share = shcore::path::join_path(_sandbox_dir, "sandbox.share");

#ifdef _WIN32
    code = "var __path_splitter = '\\\\';";
    exec_and_out_equals(code);
    auto tokens = shcore::split_string(_sandbox_dir, "\\");
    if (!tokens.at(tokens.size() - 1).empty()) tokens.push_back("");

    // The sandbox dir for C++
    _sandbox_dir = shcore::str_join(tokens, "\\");

    // The sandbox dir for JS
    code = "var __sandbox_dir = '" + shcore::str_join(tokens, "\\\\") + "';";
    exec_and_out_equals(code);

    // output sandbox dir
    _output_tokens["__output_sandbox_dir"] = shcore::str_join(tokens, "\\");

    tokens = shcore::split_string(_sandbox_share, "\\");
    code = "var __sandbox_share = '" + shcore::str_join(tokens, "\\\\") + "'";
    exec_and_out_equals(code);
#else
    code = "var __path_splitter = '/';";
    exec_and_out_equals(code);
    if (_sandbox_dir.back() != '/') {
      code = "var __sandbox_dir = '" + _sandbox_dir + "/';";
      exec_and_out_equals(code);
      code = "var __output_sandbox_dir = '" + _sandbox_dir + "/';";
      exec_and_out_equals(code);
    } else {
      code = "var __sandbox_dir = '" + _sandbox_dir + "';";
      exec_and_out_equals(code);
      code = "var __output_sandbox_dir = '" + _sandbox_dir + "';";
      exec_and_out_equals(code);
    }

    code = "var __sandbox_share = '" + _sandbox_share + "';";
    exec_and_out_equals(code);
#endif

    code = "var __uripwd = '" + user + ":" + password + "@" + host + ":" +
           m_port + "';";
    exec_and_out_equals(code);
    code = "var __mysqluripwd = '" + user + ":" + password + "@" + host + ":" +
           m_mysql_port + "';";
    exec_and_out_equals(code);

    if (_replaying)
      code = "var __replaying = true;";
    else
      code = "var __replaying = false;";
    exec_and_out_equals(code);
    if (_recording)
      code = "var __recording = true;";
    else
      code = "var __recording = false;";
    exec_and_out_equals(code);
  }
};

bool Shell_js_dba_tests::have_sandboxes = true;

TEST_F(Shell_js_dba_tests, no_active_session_error) {
  _options->wizards = false;
  reset_shell();

  execute("var c = dba.getCluster()");
  MY_EXPECT_STDERR_CONTAINS(
      "An open session is required to perform this operation.");

  wipe_all();
  execute("dba.verbose = true;");
  EXPECT_EQ("", output_handler.std_err);

  _options->wizards = true;
  reset_shell();

  execute("var c = dba.getCluster()");
  MY_EXPECT_STDERR_CONTAINS(
      "An open session is required to perform this operation.");

  wipe_all();
  execute("dba.verbose = true;");
  EXPECT_EQ("", output_handler.std_err);
}

// Sandbox specific tests should not do session replay
TEST_F(Shell_js_dba_tests, no_interactive_sandboxes) {
  _options->wizards = false;
  reset_shell();
  output_handler.set_log_level(shcore::Logger::LOG_WARNING);
  execute("dba.verbose = true;");

// Create directory with space and quotes in name to test.
#ifdef _WIN32
  std::string path_splitter = "\\";
#else
  std::string path_splitter = "/";
#endif
  // Note: not tested with " in the folder name because windows does not
  // support the creation of directories with: <>:"/\|?*
  std::string dir = _sandbox_dir + path_splitter + "foo \' bar";
  shcore::ensure_dir_exists(dir);

  // Create directory with long name (> 89 char).
  std::string dir_long = _sandbox_dir + path_splitter +
                         "01234567891123456789212345678931234567894123456789"
                         "5123456789612345678971234567898123456789";
  shcore::ensure_dir_exists(dir_long);

  // Create directory with non-ascii characteres.
  std::string dir_non_ascii =
      _sandbox_dir + path_splitter + "no_café_para_los_niños";
  shcore::ensure_dir_exists(dir_non_ascii);

  validate_interactive("dba_sandboxes.js");

  // Remove previously created directories.
  shcore::remove_directory(dir);
  shcore::remove_directory(dir_long);
  shcore::remove_directory(dir_non_ascii);
  // BUG#26393614
  std::vector<std::string> log{
      "Sandbox instances are only suitable for deploying and "
      "running on your local machine for testing purposes and are not "
      "accessible from external networks."};
  MY_EXPECT_LOG_CONTAINS(log);
}

TEST_F(Shell_js_dba_tests, interactive_deploy_instance) {
  _options->interactive = true;
  reset_shell();
  output_handler.set_log_level(shcore::Logger::LOG_WARNING);
  // BUG 26830224
  // Please enter a MySQL root password for the new instance:
  output_handler.passwords.push_back({"*", "root", {}});
  validate_interactive("dba_deploy_sandbox.js");
  // BUG#26393614
  std::vector<std::string> log{
      "Sandbox instances are only suitable for deploying and "
      "running on your local machine for testing purposes and are not "
      "accessible from external networks."};
  MY_EXPECT_LOG_CONTAINS(log);
}

TEST_F(Shell_js_dba_tests, no_interactive) {
  std::string bad_config = "[mysqld]\ngtid_mode = OFF\n";
  create_file("mybad.cnf", bad_config);

  _options->wizards = false;
  reset_replayable_shell();

  // Validates error conditions on create, get and drop cluster
  // Lets the cluster created
  validate_interactive("dba_no_interactive.js");
}

TEST_F(Shell_js_dba_tests, cluster_no_interactive) {
  _options->wizards = false;
  reset_replayable_shell();

  output_handler.set_log_level(shcore::Logger::LOG_DEBUG);

  // Tests cluster functionality, adding, removing instances
  // error conditions
  // Lets the cluster empty
  validate_interactive("dba_cluster_no_interactive.js");

  std::vector<std::string> log{
      "Creating recovery account 'mysql_innodb_cluster_"};
  MY_EXPECT_LOG_CONTAINS(log);
}

TEST_F(Shell_js_dba_tests, cluster_multimaster_no_interactive) {
  _options->wizards = false;
  reset_replayable_shell();

  // Tests cluster functionality, adding, removing instances
  // error conditions
  // Lets the cluster empty
  validate_interactive("dba_cluster_multimaster_no_interactive.js");
}

TEST_F(Shell_js_dba_tests, interactive) {
  // IMPORTANT NOTE: This test fixture requires non sandbox server as the base
  // server.
  std::string bad_config = "[mysqld]\ngtid_mode = OFF\n";
  create_file("mybad.cnf", bad_config);

  _options->interactive = true;
  reset_replayable_shell();

  // Validates error conditions on create, get and drop cluster
  // Lets the cluster created
  validate_interactive("dba_interactive.js");
}

TEST_F(Shell_js_dba_tests, cluster_interactive) {
  _options->interactive = true;
  reset_replayable_shell();

  // we want to catch log_info output
  output_handler.set_log_level(shcore::Logger::LOG_INFO);

  //@ Cluster: removeInstance errors
  output_handler.prompts.push_back({"*", "no", {}});
  output_handler.prompts.push_back({"*", "yes", {}});

  // @<OUT> Cluster: dissolve error: not empty
  output_handler.prompts.push_back({"*", "no", {}});

  //@# Cluster: rejoin_instance with interaction, error
  output_handler.passwords.push_back({"*", "n", {}});

  //@# Cluster: rejoin_instance with interaction, error 2
  output_handler.passwords.push_back({"*", "n", {}});

  // @<OUT> Cluster: rejoinInstance with interaction, ok
  output_handler.passwords.push_back({"*", "root", {}});

  // @<OUT> Cluster: final dissolve
  output_handler.prompts.push_back({"*", "yes", {}});

  // Tests cluster functionality, adding, removing instances
  // error conditions
  // Lets the cluster empty
  validate_interactive("dba_cluster_interactive.js");

  std::vector<std::string> log{
      "Creating recovery account 'mysql_innodb_cluster_"};
  MY_EXPECT_LOG_CONTAINS(log);
}

TEST_F(Shell_js_dba_tests, cluster_multimaster_interactive) {
  _options->interactive = true;
  reset_replayable_shell();

  //@<OUT> Dba: createCluster multiPrimary with interaction, cancel
  output_handler.prompts.push_back({"*", "no", {}});

  //@<OUT> Dba: createCluster multiPrimary with interaction, ok
  output_handler.prompts.push_back({"*", "yes", {}});

  //@ Dissolve cluster
  output_handler.prompts.push_back({"*", "yes", {}});

  //@<OUT> Dba: createCluster multiMaster with interaction, regression for
  // BUG#25926603
  output_handler.prompts.push_back({"*", "yes", {}});

  //@ Dissolve cluster with success
  output_handler.prompts.push_back({"*", "yes", {}});

  //@<OUT> Dba: createCluster multiMaster with interaction 2, ok
  output_handler.prompts.push_back({"*", "yes", {}});

  //@: Cluster: rejoinInstance errors
  output_handler.passwords.push_back({"*", "root", {}});
  output_handler.passwords.push_back({"*", "root", {}});

  // @<OUT> Cluster: status for rejoin: success
  // Dissolve cluster.
  output_handler.prompts.push_back({"*", "yes", {}});

  // Tests cluster functionality, adding, removing instances
  // error conditions.
  validate_interactive("dba_cluster_multimaster_interactive.js");
}

TEST_F(Shell_js_dba_tests, cluster_no_misconfigurations) {
  _options->wizards = false;
  reset_replayable_shell();
  output_handler.set_log_level(shcore::Logger::LOG_WARNING);

  validate_interactive("dba_cluster_no_misconfigurations.js");

  std::vector<std::string> log = {
      // "DBA: root@localhost:" + _mysql_sandbox_port1 +
      //     " : Server variable binlog_format was changed from 'MIXED' to
      //     'ROW'",
      "DBA: root@localhost:" + std::to_string(_mysql_sandbox_ports[0]) +
      " : Server variable binlog_checksum was changed from 'CRC32' to "
      "'NONE'"};

  MY_EXPECT_LOG_NOT_CONTAINS(log);
}

TEST_F(Shell_js_dba_tests, cluster_no_misconfigurations_interactive) {
  _options->interactive = true;
  reset_replayable_shell();

  output_handler.set_log_level(shcore::Logger::LOG_WARNING);

  validate_interactive("dba_cluster_no_misconfigurations_interactive.js");

  std::vector<std::string> log = {
      // "DBA: root@localhost:" + _mysql_sandbox_port1 +
      //     " : Server variable binlog_format was changed from 'MIXED' to
      //     'ROW'",
      "DBA: root@localhost:" + std::to_string(_mysql_sandbox_ports[0]) +
      " : Server variable binlog_checksum was changed from 'CRC32' to "
      "'NONE'"};

  MY_EXPECT_LOG_NOT_CONTAINS(log);
}

TEST_F(Shell_js_dba_tests, dba_cluster_mts) {
  _options->wizards = false;
  reset_replayable_shell();

  std::string bad_config = "[mysqld]\ngtid_mode = OFF\n";
  create_file("mybad.cnf", bad_config);

  validate_interactive("dba_cluster_mts.js");
}

TEST_F(Shell_js_dba_tests, super_read_only_handling) {
  reset_replayable_shell();
  //@<OUT> Configures the instance, answers 'yes' on the read only prompt
  output_handler.prompts.push_back({"*", "y", {}});

  //@ Reboot the cluster
  // Confirms addition of second instance
  output_handler.prompts.push_back({"*", "y", {}});

  // Confirms addition of third instance
  output_handler.prompts.push_back({"*", "y", {}});

  validate_interactive("dba_super_read_only_handling.js");
}

}  // namespace shcore
