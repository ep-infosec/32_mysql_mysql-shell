/*
 * Copyright (c) 2016, 2022, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_BASE_CLUSTER_IMPL_H_
#define MODULES_ADMINAPI_COMMON_BASE_CLUSTER_IMPL_H_

#include <list>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "scripting/types.h"

#include "modules/adminapi/common/api_options.h"
#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/mysql/lock_service.h"

namespace mysqlsh {
namespace dba {

// User provided option for telling us to assume that the cluster was created
// with a server where the full update history is reflected in its GTID set
constexpr const char k_cluster_attribute_assume_gtid_set_complete[] =
    "opt_gtidSetIsComplete";

// host (as in user@host) to use when creating managed replication accounts
constexpr const char k_cluster_attribute_replication_allowed_host[] =
    "opt_replicationAllowedHost";

constexpr const char k_cluster_attribute_transaction_size_limit[] =
    "opt_transactionSizeLimit";

// Cluster capabilities
constexpr const char k_cluster_capabilities[] = "capabilities";

class Base_cluster_impl {
 public:
  Base_cluster_impl(const std::string &cluster_name,
                    std::shared_ptr<Instance> group_server,
                    std::shared_ptr<MetadataStorage> metadata_storage);
  virtual ~Base_cluster_impl();

  virtual Cluster_type get_type() const = 0;

  virtual std::string get_topology_type() const = 0;

  bool get_gtid_set_is_complete() const;

  Cluster_id get_id() const { return m_id; }
  void set_id(Cluster_id id) { m_id = id; }

  std::string get_description() const { return m_description; }

  void set_description(const std::string &description) {
    m_description = description;
  }

  std::string get_name() const { return m_cluster_name; }

  void set_cluster_name(const std::string &name) { m_cluster_name = name; }
  const std::string &cluster_name() const { return m_cluster_name; }

  void set_target_server(const std::shared_ptr<Instance> &instance);

  std::shared_ptr<Instance> get_cluster_server() const {
    return m_cluster_server;
  }

  std::shared_ptr<Instance> get_primary_master() const {
    return m_primary_master;
  }

  void check_preconditions(const std::string &function_name,
                           Function_availability *custom_func_avail = nullptr);

  std::shared_ptr<MetadataStorage> get_metadata_storage() const {
    return m_metadata_storage;
  }

  const mysqlshdk::mysql::Auth_options &default_admin_credentials() const {
    return m_admin_credentials;
  }

  virtual mysqlsh::dba::Instance *acquire_primary(
      mysqlshdk::mysql::Lock_mode mode = mysqlshdk::mysql::Lock_mode::NONE,
      const std::string &skip_lock_uuid = "") = 0;

  virtual void release_primary(mysqlsh::dba::Instance *primary = nullptr) = 0;

  virtual void disconnect();

  virtual shcore::Value list_routers(bool only_upgrade_required);

  virtual void setup_admin_account(const std::string &username,
                                   const std::string &host,
                                   const Setup_account_options &options);

  virtual void setup_router_account(const std::string &username,
                                    const std::string &host,
                                    const Setup_account_options &options);

  virtual void remove_router_metadata(const std::string &router);

  void set_instance_tag(const std::string &instance_def,
                        const std::string &option, const shcore::Value &value);

  void set_cluster_tag(const std::string &option, const shcore::Value &value);

  void set_instance_option(const std::string &instance_def,
                           const std::string &option,
                           const shcore::Value &value);

  void set_option(const std::string &option, const shcore::Value &value);

  /**
   * Get the tags for a specific Cluster/ReplicaSet
   *
   * This function gets the tags for a Cluster/ReplicaSet and for its members
   *
   * @return a shcore::Value containing a dictionary object with the command
   * output
   */
  shcore::Value get_cluster_tags() const;

 public:
  /*
   * Synchronize transactions on target instance.
   *
   * Wait for all current cluster transactions to be applied on the specified
   * target instance. Function will monitor for replication errors on the named
   * channel and throw an exception if an error is detected.
   *
   * @param target_instance instance to wait for transaction to be applied.
   * @param channel_name the name of the replication channel to monitor for I/O
   * and SQL errors
   * @param timeout number of seconds to wait
   *
   * @throw RuntimeError if the timeout is reached when waiting for
   * transactions to be applied or replication errors are detected.
   */
  void sync_transactions(const mysqlshdk::mysql::IInstance &target_instance,
                         const std::string &channel_name, int timeout) const;

  /**
   * Connect to the given instance specification given, while validating its
   * syntax.
   *
   * @param instance_def the instance to connect to, as a host:port string.
   * @param print_error boolean value to indicate whether an error shall be
   * printed or not
   * @param allow_account_override if true, allows the user to override the
   * account, otherwise they must match the cluster session
   *
   * A URL is allowed, if it matches that of m_cluster_server.
   * @return instance object owned by ipool
   */
  std::shared_ptr<Instance> connect_target_instance(
      const std::string &instance_def, bool print_error = true,
      bool allow_account_override = false);

  std::shared_ptr<Instance> connect_target_instance(
      const mysqlshdk::db::Connection_options &instance_def,
      bool print_error = true, bool allow_account_override = false);

  std::string make_replication_user_name(uint32_t server_id,
                                         const std::string &user_prefix) const;

 protected:
  Cluster_id m_id;
  std::string m_cluster_name;
  std::string m_description;

  // Session to the primary member of the Cluster/ReplicaSet upon the
  // initialization of the corresponding objects
  std::shared_ptr<Instance> m_cluster_server;

  // Session to the primary instance
  // NOTE: In standalone Clusters it's the primary member of the Cluster, but if
  // the Cluster belongs to a ClusterSet, it's the primary member of the primary
  // Cluster
  std::shared_ptr<Instance> m_primary_master;

  std::shared_ptr<MetadataStorage> m_metadata_storage;

  mysqlshdk::mysql::Auth_options m_admin_credentials;

  enum class Setup_account_type { ADMIN, ROUTER };

  void setup_account_common(const std::string &username,
                            const std::string &host,
                            const Setup_account_options &options,
                            const Setup_account_type &type);

  virtual void _set_instance_option(const std::string &instance_def,
                                    const std::string &option,
                                    const shcore::Value &value) = 0;

  virtual void _set_option(const std::string &option,
                           const shcore::Value &value) = 0;

  void target_server_invalidated();

  /**
   * Does simple validation on the option provided to setOption and
   * setInstanceOption methods and splits the option into a pair: namespace,
   * option_name. If dealing with a built-in tag, it also converts the value to
   * the expected type. If the namespace is empty, it means the option has no
   * namespace.
   * @param option the option parameter of the setOption and setInstance option
   * methods.
   * @param value the value that was provided for the option. We use it to
   * validate if built-in tags are of the expected type or can be converted to
   * it.
   * @return a tuple with strings and value: namespace, option_name, value.
   * @throws argumentError if the format of the option is invalid, or using an
   * unsupported namespace.
   */
  std::tuple<std::string, std::string, shcore::Value>
  validate_set_option_namespace(const std::string &option,
                                const shcore::Value &value) const;
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_COMMON_BASE_CLUSTER_IMPL_H_
