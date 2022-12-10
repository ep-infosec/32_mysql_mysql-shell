/*
 * Copyright (c) 2018, 2022, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/validations.h"

#include <string>

#include "modules/adminapi/common/accounts.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/dba/check_instance.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dba {

void ensure_gr_instance_configuration_valid(
    mysqlshdk::mysql::IInstance *target_instance, bool full,
    bool skip_check_tables_pk) {
  auto console = mysqlsh::current_console();

  shcore::Value result;

  if (full) {
    console->print_info(
        "Validating instance configuration at " +
        target_instance->get_connection_options().uri_endpoint() + "...");

    Check_instance check(target_instance->get_connection_options(), "", true,
                         skip_check_tables_pk);
    check.prepare();
    result = check.execute();
    check.finish();

    if (!result || result.as_map()->at("status").get_string() == "ok") {
      console->print_info("Instance configuration is suitable.");
    } else {
      console->print_error(
          "Instance must be configured and validated with "
          "dba.<<<checkInstanceConfiguration>>>() and "
          "dba.<<<configureInstance>>>() "
          "before it can be used in an InnoDB cluster.");
      throw shcore::Exception::runtime_error("Instance check failed");
    }
  } else {
    checks::validate_host_address(*target_instance, 0);
  }

  validate_replication_filters(*target_instance,
                               Cluster_type::GROUP_REPLICATION);
}

namespace {

void validate_ar_configuration(mysqlshdk::mysql::IInstance *target_instance) {
  auto console = mysqlsh::current_console();

  // Perform check with no update
  bool restart = false;
  bool config_file_change = false;
  bool sysvar_change = false;
  shcore::Value ret_val;

  auto cfg = mysqlsh::dba::create_server_config(
      target_instance, mysqlshdk::config::k_dft_cfg_server_handler, true);

  if (!checks::validate_configuration(
           target_instance, "", cfg.get(), Cluster_type::ASYNC_REPLICATION,
           true, &restart, &config_file_change, &sysvar_change, &ret_val)
           .empty()) {
    if (restart && !config_file_change && !sysvar_change) {
      console->print_note(target_instance->descr() +
                          ": Please restart the MySQL server and try again.");
    } else {
      console->print_error(target_instance->descr() +
                           ": Instance must be configured and validated with "
                           "dba.<<<configureReplicaSetInstance>>>() "
                           "before it can be used in a replicaset.");
    }
    throw shcore::Exception("Instance check failed",
                            SHERR_DBA_INVALID_SERVER_CONFIGURATION);
  } else {
    console->print_info(target_instance->descr() +
                        ": Instance configuration is suitable.");
  }
}

}  // namespace

void ensure_ar_instance_configuration_valid(
    mysqlshdk::mysql::IInstance *target_instance) {
  auto console = mysqlsh::current_console();

  checks::validate_host_address(*target_instance, 1);

  validate_ar_configuration(target_instance);

  validate_replication_filters(*target_instance,
                               Cluster_type::ASYNC_REPLICATION);
}

void ensure_user_privileges(const mysqlshdk::mysql::IInstance &instance,
                            Cluster_type purpose) {
  std::string current_user, current_host;
  log_debug("Checking user privileges");
  // Get the current user/host
  instance.get_current_user(&current_user, &current_host);

  std::string error_info;
  if (!validate_cluster_admin_user_privileges(
          instance, current_user, current_host, purpose, &error_info)) {
    auto console = mysqlsh::current_console();
    console->print_error(error_info);
    console->print_info("For more information, see the online documentation.");

    auto msg = shcore::str_format(
        "The account %s is missing privileges required to manage an "
        "%s.",
        shcore::make_account(current_user, current_host).c_str(),
        to_display_string(purpose, Display_form::THING_FULL).c_str());

    throw shcore::Exception::runtime_error(msg);
  }
}

bool ensure_gtid_sync_possible(const mysqlshdk::mysql::IInstance &master,
                               const mysqlshdk::mysql::IInstance &instance,
                               bool fatal) {
  std::string missing_gtids = master.queryf_one_string(
      0, "", "SELECT GTID_SUBTRACT(@@global.gtid_purged, ?)",
      mysqlshdk::mysql::get_executed_gtid_set(instance));

  if (!missing_gtids.empty()) {
    log_info("Transactions missing at %s that were purged from primary %s: %s",
             instance.descr().c_str(), master.descr().c_str(),
             missing_gtids.c_str());

    std::string msg = instance.descr() + " is missing " +
                      std::to_string(mysqlshdk::mysql::estimate_gtid_set_size(
                          missing_gtids)) +
                      " transactions that have been purged from the "
                      "current PRIMARY (" +
                      master.descr() + ")";

    if (fatal) {
      current_console()->print_error(msg);
      throw shcore::Exception(
          "Missing purged transactions at " + instance.descr(),
          SHERR_DBA_DATA_RECOVERY_NOT_POSSIBLE);
    } else {
      current_console()->print_warning(msg);
      return false;
    }
  }
  return true;
}

bool ensure_gtid_no_errants(const mysqlshdk::mysql::IInstance &master,
                            const mysqlshdk::mysql::IInstance &instance,
                            bool fatal) {
  std::string slave_gtid_executed =
      mysqlshdk::mysql::get_executed_gtid_set(instance);

  std::string errant_gtids = master.queryf_one_string(
      0, "", "SELECT GTID_SUBTRACT(?, @@global.gtid_executed)",
      slave_gtid_executed);

  if (!errant_gtids.empty()) {
    log_info("Errant transactions at %s compared to %s: %s",
             instance.descr().c_str(), master.descr().c_str(),
             errant_gtids.c_str());

    std::string msg =
        instance.descr() + " has " +
        std::to_string(mysqlshdk::mysql::estimate_gtid_set_size(errant_gtids)) +
        " errant transactions that have not originated from the current "
        "PRIMARY (" +
        master.descr() + ")";

    if (fatal) {
      current_console()->print_error(msg);
      throw shcore::Exception("Errant transactions at " + instance.descr(),
                              SHERR_DBA_DATA_ERRANT_TRANSACTIONS);
    } else {
      current_console()->print_warning(msg);
      return false;
    }
  }
  return true;
}

void ensure_certificates_set(const mysqlshdk::mysql::IInstance &instance,
                             const Cluster_ssl_mode &ssl_mode) {
  auto console = mysqlsh::current_console();

  if (ssl_mode == Cluster_ssl_mode::VERIFY_CA ||
      ssl_mode == Cluster_ssl_mode::VERIFY_IDENTITY) {
    // Get the values of ssl_ca and ss_capath
    std::string ssl_ca = instance.get_sysvar_string("ssl_ca").get_safe("");
    std::string ssl_capath =
        instance.get_sysvar_string("ssl_capath").get_safe("");

    // If both unset, error-out
    if (ssl_ca.empty() && ssl_capath.empty()) {
      console->print_error(
          "CA certificates options not set. ssl_ca or ssl_capath are "
          "required, to supply a CA certificate that matches the one used by "
          "the server.");
      throw std::runtime_error(
          "memberSslMode '" + to_string(ssl_mode) +
          "' requires Certificate Authority (CA) certificates "
          "to be supplied.");
    }
  }
}

void check_protocol_upgrade_possible(
    const mysqlshdk::mysql::IInstance &group_instance,
    const std::string &skip_server_uuid) {
  try {
    mysqlshdk::utils::Version target_protocol_version;

    if (mysqlshdk::gr::is_protocol_upgrade_possible(
            group_instance, skip_server_uuid, &target_protocol_version)) {
      current_console()->print_note(
          "Group Replication protocol can be upgraded to version " +
          target_protocol_version.get_full() +
          ". You can use Cluster.rescan({upgradeCommProtocol:true}) to upgrade "
          "it.");
    }
  } catch (const shcore::Exception &error) {
    // The UDF may fail with MySQL Error 1123 if any of the members is
    // RECOVERING In such scenario, we must abort the upgrade protocol version
    // process and warn the user
    if (error.code() == ER_CANT_INITIALIZE_UDF) {
      auto console = mysqlsh::current_console();
      console->print_note(
          "Unable to determine the Group Replication protocol version, while "
          "verifying if a protocol upgrade would be possible: " +
          std::string(error.what()) + ".");
    } else {
      throw;
    }
  }
}

}  // namespace dba
}  // namespace mysqlsh
