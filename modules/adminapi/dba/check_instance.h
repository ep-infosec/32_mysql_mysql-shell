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

#ifndef MODULES_ADMINAPI_DBA_CHECK_INSTANCE_H_
#define MODULES_ADMINAPI_DBA_CHECK_INSTANCE_H_

#include <memory>
#include <string>

#include "modules/adminapi/common/instance_pool.h"
#include "modules/adminapi/common/provisioning_interface.h"
#include "modules/adminapi/dba/configure_instance.h"
#include "modules/command_interface.h"
#include "mysqlshdk/include/scripting/types_cpp.h"

namespace mysqlsh {
namespace dba {

class Check_instance : public Command_interface {
 public:
  Check_instance(const mysqlshdk::db::Connection_options &instance_cnx_opts,
                 const std::string &verify_mycnf_path, bool silent = false,
                 bool skip_check_tables_pk = false);
  ~Check_instance();

  void prepare() override;
  shcore::Value execute() override;
  void rollback() override;
  void finish() override;

 private:
  void check_instance_address();
  bool check_schema_compatibility();
  bool check_configuration();
  void check_clone_plugin_status();

  /**
   * Prepare the internal mysqlshdk::config::Config object to use.
   *
   * Initialize the mysqlshdk::config::Config object adding the proper
   * configuration handlers based on the target instance settings (e.g., server
   * version) and operation input parameters.
   */
  void prepare_config_object();

 private:
  const mysqlshdk::db::Connection_options m_instance_cnx_opts;
  std::string m_mycnf_path;
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  bool m_is_valid = false;
  mysqlshdk::utils::nullable<bool> m_can_set_persist;
  shcore::Value m_ret_val;

  const bool m_silent;
  const bool m_skip_check_tables_pk{false};

  // Configuration object (to read and set instance configurations).
  std::unique_ptr<mysqlshdk::config::Config> m_cfg;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_DBA_CHECK_INSTANCE_H_
