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

#ifndef MODULES_ADMINAPI_CLUSTER_STATUS_H_
#define MODULES_ADMINAPI_CLUSTER_STATUS_H_

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/command_interface.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/utils/utils_net.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

typedef std::map<std::string, std::pair<mysqlshdk::db::Row_by_name,
                                        mysqlshdk::db::Row_by_name>>
    Member_stats_map;

struct Instance_metadata_info {
  Instance_metadata md;
  std::string actual_server_uuid;
};

class Status : public Command_interface {
 public:
  Status(const Cluster_impl &cluster,
         const mysqlshdk::utils::nullable<uint64_t> &extended);

  ~Status() override;

  /**
   * Prepare the cluster Status command for execution.
   * Validates parameters and others, more specifically:
   *   - Ensure the cluster is still registered in the metadata
   *   - Ensure the topology type didn't change as registered in the metadata
   *   - Gets the current members list
   *   - Connects to every Cluster member and populates the internal
   * connection lists
   */
  void prepare() override;

  /**
   * Execute the cluster status command.
   * More specifically:
   * - Lists the status of the Cluster and its Instances.
   *
   * @return an shcore::Value containing a dictionary object with the command
   * output
   */
  shcore::Value execute() override;

  /**
   * Rollback the command.
   *
   * NOTE: Not currently used (does nothing).
   */
  void rollback() override;

  /**
   * Finalize the command execution.
   * More specifically:
   * - Reset all auxiliary (temporary) data used for the operation execution.
   */
  void finish() override;

 private:
  const Cluster_impl &m_cluster;
  mysqlshdk::utils::nullable<uint64_t> m_extended;

  shcore::Value get_default_replicaset_status();

  std::vector<Instance_metadata> m_instances;
  std::unordered_map<std::string, std::shared_ptr<Instance>,
                     std::hash<std::string>,
                     mysqlshdk::utils::Endpoint_comparer>
      m_member_sessions;
  std::unordered_map<std::string, std::string, std::hash<std::string>,
                     mysqlshdk::utils::Endpoint_comparer>
      m_member_connect_errors;

  bool m_no_quorum = false;
  std::optional<int64_t> m_cluster_transaction_size_limit = -1;

  void connect_to_members();

  shcore::Dictionary_t check_group_status(
      const mysqlsh::dba::Instance &instance,
      const std::vector<mysqlshdk::gr::Member> &members, bool has_quorum);

  const Instance_metadata *instance_with_uuid(const std::string &uuid);

  Member_stats_map query_member_stats();

  static void collect_last_error(shcore::Dictionary_t dict,
                                 const mysqlshdk::db::Row_ref_by_name &row,
                                 const std::string &prefix = "LAST_",
                                 const std::string &key_prefix = "last");

  static shcore::Value collect_last(const mysqlshdk::db::Row_ref_by_name &row,
                                    const std::string &prefix,
                                    const std::string &what);

  static shcore::Value collect_current(
      const mysqlshdk::db::Row_ref_by_name &row, const std::string &prefix,
      const std::string &what);

  shcore::Value connection_status(const mysqlshdk::db::Row_ref_by_name &row);

  shcore::Value coordinator_status(const mysqlshdk::db::Row_ref_by_name &row);

  shcore::Value applier_status(const mysqlshdk::db::Row_ref_by_name &row);

  void collect_basic_local_status(shcore::Dictionary_t dict,
                                  const mysqlsh::dba::Instance &instance,
                                  bool is_primary);

  void collect_local_status(shcore::Dictionary_t dict,
                            const mysqlsh::dba::Instance &instance,
                            bool recovering);

  void feed_metadata_info(shcore::Dictionary_t dict,
                          const Instance_metadata &info);

  void feed_member_info(shcore::Dictionary_t dict,
                        const mysqlshdk::gr::Member &member,
                        const mysqlshdk::utils::nullable<bool> &offline_mode,
                        const mysqlshdk::utils::nullable<bool> &super_read_only,
                        const std::vector<std::string> &fence_sysvars,
                        mysqlshdk::gr::Member_state self_state,
                        bool is_auto_rejoin_running);

  void feed_member_stats(shcore::Dictionary_t dict,
                         const mysqlshdk::db::Row_by_name &stats);

  shcore::Dictionary_t get_topology(
      const std::vector<mysqlshdk::gr::Member> &member_info);

  shcore::Dictionary_t collect_replicaset_status();
};

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_CLUSTER_STATUS_H_
