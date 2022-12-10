/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster/check_instance_state.h"

#include <memory>
#include "modules/adminapi/common/errors.h"
#include "modules/adminapi/common/instance_validations.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/sql.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/clone.h"
#include "mysqlshdk/libs/mysql/replication.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

Check_instance_state::Check_instance_state(
    const Cluster_impl &cluster,
    const std::shared_ptr<mysqlsh::dba::Instance> &target)
    : m_cluster(cluster), m_target_instance(target) {
  m_address_in_metadata = m_target_instance->get_canonical_address();
}

Check_instance_state::~Check_instance_state() {}

/**
 * Ensure target instance has a valid GR state, being the only accepted state
 * Standalone.
 *
 * Throws an explicative runtime error if the target instance is not Standalone
 */
void Check_instance_state::ensure_instance_valid_gr_state() {
  // Get the instance GR state
  TargetType::Type instance_type = get_gr_instance_type(*m_target_instance);

  if (instance_type != TargetType::Standalone) {
    std::string error = "The instance '" + m_target_instance->descr();

    // No need to verify if the state is TargetType::InnoDBCluster because
    // that has been verified in ensure_instance_not_belong_to_cluster

    if (instance_type == TargetType::GroupReplication) {
      error +=
          "' belongs to a Group Replication group that is not managed as an "
          "InnoDB cluster.";
    } else if (instance_type == TargetType::StandaloneWithMetadata) {
      error +=
          "' is a standalone instance but is part of a different InnoDB "
          "Cluster (metadata exists, instance does not belong to that "
          "metadata, and Group Replication is not active).";
    } else if (instance_type == TargetType::StandaloneInMetadata) {
      error +=
          "' is a standalone instance but is part of a different InnoDB "
          "Cluster (metadata exists, instance belongs to that metadata, but "
          "Group Replication is not active).";
    } else {
      error += " has an unknown state.";
    }

    throw shcore::Exception::runtime_error(error);
  }
}

/**
 * Get the target instance GTID state in relation to the Cluster
 *
 * This function gets the instance GTID state and builds a Dictionary with the
 * format:
 * {
 *   "reason": "currentReason",
 *   "state": "currentState",
 * }
 *
 * @return a shcore::Dictionary_t containing a dictionary object with instance
 * GTID state in relation to the cluster
 */
shcore::Dictionary_t Check_instance_state::collect_instance_state() {
  std::shared_ptr<Instance> cluster_instance = m_cluster.get_cluster_server();

  // Check the gtid state in regards to the cluster_session
  mysqlshdk::mysql::Replica_gtid_state state =
      mysqlshdk::mysql::check_replica_gtid_state(
          *cluster_instance, *m_target_instance, nullptr, nullptr);

  std::string reason;
  std::string status;
  switch (state) {
    case mysqlshdk::mysql::Replica_gtid_state::DIVERGED:
      if (!m_clone_available) {
        status = "error";
      } else {
        status = "warning";
      }
      reason = "diverged";
      break;
    case mysqlshdk::mysql::Replica_gtid_state::IRRECOVERABLE:
      if (!m_clone_available) {
        status = "error";
      } else {
        status = "warning";
      }
      reason = "lost_transactions";
      break;
    case mysqlshdk::mysql::Replica_gtid_state::RECOVERABLE:
    case mysqlshdk::mysql::Replica_gtid_state::IDENTICAL:
      status = "ok";
      reason = "recoverable";
      break;
    case mysqlshdk::mysql::Replica_gtid_state::NEW:
      status = "ok";
      reason = "new";
      break;
  }

  // Check if the GTIDs were purged from the whole cluster
  bool all_purged = false;

  m_cluster.execute_in_members(
      {mysqlshdk::gr::Member_state::ONLINE},
      m_target_instance->get_connection_options(), {},
      [&all_purged, this](const std::shared_ptr<Instance> &instance,
                          const mysqlshdk::gr::Member &) {
        // Get the gtid state in regards to the cluster_session
        mysqlshdk::mysql::Replica_gtid_state gtid_state =
            mysqlshdk::mysql::check_replica_gtid_state(
                *instance, *m_target_instance, nullptr, nullptr);

        if (gtid_state == mysqlshdk::mysql::Replica_gtid_state::IRRECOVERABLE) {
          all_purged = true;
        } else {
          all_purged = false;
        }

        return all_purged;
      });

  // If GTIDs were purged on all members, report that
  // If clone is available, the status shall be warning
  if (all_purged) {
    if (!m_clone_available) {
      status = "error";
    } else {
      status = "warning";
    }
    reason = "all_purged";
  }

  shcore::Dictionary_t ret = shcore::make_dict();
  (*ret)["state"] = shcore::Value(status);
  (*ret)["reason"] = shcore::Value(reason);
  return ret;
}

/**
 * Print the instance GTID state information
 *
 * This function prints human-readable information of the instance GTID state in
 * relation to the Cluster based a shcore::Dictionary_t with that
 * information (collected by collect_instance_state())
 *
 * @param instance_state shcore::Dictionary_t containing a dictionary object
 * with instance GTID state in relation to the cluster
 *
 */
void Check_instance_state::print_instance_state_info(
    shcore::Dictionary_t instance_state) {
  auto console = mysqlsh::current_console();

  if (instance_state->get_string("state") == "ok") {
    console->print_info("The instance '" + m_target_instance->descr() +
                        "' is valid for the cluster.");

    if (instance_state->get_string("reason") == "new") {
      console->print_info("The instance is new to Group Replication.");
    } else {
      console->print_info("The instance is fully recoverable.");
    }
  } else {
    if (instance_state->get_string("reason") == "diverged") {
      if (m_clone_available) {
        console->print_info(
            "The instance contains additional transactions in relation to "
            "the cluster. However, Clone is available and if desired can be "
            "used to overwrite the data and add the instance to a cluster.");
      } else {
        console->print_info("The instance '" + m_target_instance->descr() +
                            "' is invalid for the cluster.");
        console->print_info(
            "The instance contains additional transactions in relation to "
            "the cluster.");
      }
    } else {
      // Check if clone is supported
      // BUG#29630591: CLUSTER.CHECKINSTANCESTATE() INCORRECT OUTPUT WHEN BINARY
      // LOGS WERE PURGED
      if (m_clone_available) {
        if (instance_state->get_string("reason") == "lost_transactions") {
          console->print_info(
              "There are transactions in the cluster that can't be recovered "
              "on the instance, however, Clone is available and can be used "
              "when adding it to a cluster.");
        } else {
          console->print_info(
              "The cluster transactions cannot be recovered "
              "on the instance, however, Clone is available and can be used "
              "when adding it to a cluster.");
        }
      } else {
        console->print_info("The instance '" + m_target_instance->descr() +
                            "' is invalid for the cluster.");

        if (instance_state->get_string("reason") == "lost_transactions") {
          console->print_info(
              "There are transactions in the cluster that can't be recovered "
              "on the instance.");
        } else {
          console->print_info(
              "The cluster transactions cannot be recovered on the instance.");
        }
      }
    }
  }
  console->print_info();
}

void Check_instance_state::prepare() {
  // Ensure the target instance does not belong to the metadata.
  mysqlsh::dba::checks::ensure_instance_not_belong_to_metadata(
      *m_target_instance, m_address_in_metadata, m_cluster);

  // Ensure the target instance has a valid GR state
  ensure_instance_valid_gr_state();

  // Check if clone is available;
  m_clone_available =
      mysqlshdk::mysql::is_clone_available(*m_target_instance) &&
      !m_cluster.get_disable_clone_option();
}

shcore::Value Check_instance_state::execute() {
  auto console = mysqlsh::current_console();

  console->print_info("Analyzing the instance '" + m_target_instance->descr() +
                      "' replication state...");
  console->print_info();

  // Get the instance state
  shcore::Dictionary_t instance_state;

  instance_state = collect_instance_state();

  // Print the instance state information
  print_instance_state_info(instance_state);

  return shcore::Value(instance_state);
}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
