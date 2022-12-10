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

#ifndef MODULES_ADMINAPI_MOD_DBA_CLUSTER_H_
#define MODULES_ADMINAPI_MOD_DBA_CLUSTER_H_

#include <memory>
#include <string>

#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "shellcore/shell_options.h"

#include "modules/adminapi/cluster/api_options.h"
#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/cluster_set/api_options.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/group_replication_options.h"
#include "modules/adminapi/mod_dba_cluster_set.h"
#include "mysqlshdk/libs/db/connection_options.h"

namespace mysqlsh {
namespace dba {

/**
 * \ingroup AdminAPI
 * $(CLUSTER_BRIEF)
 *
 * $(CLUSTER_DETAIL)
 */
class Cluster : public std::enable_shared_from_this<Cluster>,
                public shcore::Cpp_object_bridge {
 public:
#if DOXYGEN_JS
  String name;  //!< $(CLUSTER_GETNAME_BRIEF)
  Undefined addInstance(InstanceDef instance, Dictionary options);
  Dictionary checkInstanceState(InstanceDef instance);
  ClusterSet createClusterSet(String domainName, Dictionary options);
  ClusterSet getClusterSet();
  String describe();
  Undefined disconnect();
  Undefined dissolve(Dictionary options);
  Undefined forceQuorumUsingPartitionOf(InstanceDef instance, String password);
  String getName();
  Dictionary listRouters(Dictionary options);
  Dictionary rejoinInstance(InstanceDef instance, Dictionary options);
  Undefined removeInstance(InstanceDef instance, Dictionary options);
  Undefined rescan(Dictionary options);
  Undefined resetRecoveryAccountsPassword(Dictionary options);
  String status(Dictionary options);
  Undefined switchToSinglePrimaryMode(InstanceDef instance);
  Undefined switchToMultiPrimaryMode();
  Undefined setPrimaryInstance(InstanceDef instance, Dictionary options);
  String options(Dictionary options);
  Undefined setOption(String option, String value);
  Undefined setInstanceOption(InstanceDef instance, String option,
                              String value);
  Boolean removeRouterMetadata(RouterDef routerDef);
  Undefined setupAdminAccount(String user, Dictionary options);
  Undefined setupRouterAccount(String user, Dictionary options);
  Undefined fenceAllTraffic();
  Undefined fenceWrites();
  Undefined unfenceWrites();
#elif DOXYGEN_PY
  str name;  //!< $(CLUSTER_GETNAME_BRIEF)
  None add_instance(InstanceDef instance, dict options);
  dict check_instance_state(InstanceDef instance);
  ClusterSet create_cluster_set(str domainName, dict options);
  ClusterSet get_cluster_set();
  str describe();
  None disconnect();
  None dissolve(dict options);
  None force_quorum_using_partition_of(InstanceDef instance, str password);
  str get_name();
  dict list_routers(dict options);
  dict rejoin_instance(InstanceDef instance, dict options);
  None remove_instance(InstanceDef instance, dict options);
  None rescan(dict options);
  None reset_recovery_accounts_password(dict options);
  str status(dict options);
  None switch_to_single_primary_mode(InstanceDef instance);
  None switch_to_multi_primary_mode();
  None set_primary_instance(InstanceDef instance, dict options);
  str options(dict options);
  None set_option(str option, str value);
  None set_instance_option(InstanceDef instance, str option, str value);
  bool remove_router_metadata(RouterDef routerDef);
  None setup_admin_account(str user, dict options);
  None setup_router_account(str user, dict options);
  None fence_all_traffic();
  None fence_writes();
  None unfence_writes();
#endif

  explicit Cluster(const std::shared_ptr<Cluster_impl> &impl);

  virtual ~Cluster();

  std::string class_name() const override { return "Cluster"; }
  std::string &append_descr(std::string &s_out, int indent = -1,
                            int quote_strings = 0) const override;
  void append_json(shcore::JSON_dumper &dumper) const override;
  bool operator==(const Object_bridge &other) const override;

  shcore::Value call(const std::string &name,
                     const shcore::Argument_list &args) override;
  shcore::Value get_member(const std::string &prop) const override;

  std::shared_ptr<Cluster_impl> impl() const { return m_impl; }

  void assert_valid(const std::string &option_name) const;

  void invalidate() { m_invalidated = true; }

  void add_instance(const Connection_options &instance_def,
                    const shcore::Option_pack_ref<cluster::Add_instance_options>
                        &options = {});
  void rejoin_instance(
      const Connection_options &instance_def,
      const shcore::Option_pack_ref<cluster::Rejoin_instance_options> &options =
          {});
  void remove_instance(
      const Connection_options &instance_def,
      const shcore::Option_pack_ref<cluster::Remove_instance_options> &options =
          {});
  shcore::Value get_replicaset(const shcore::Argument_list &args);
  shcore::Value describe(void);
  shcore::Value status(
      const shcore::Option_pack_ref<cluster::Status_options> &options);
  shcore::Dictionary_t list_routers(
      const shcore::Option_pack_ref<List_routers_options> &options);
  void dissolve(
      const shcore::Option_pack_ref<Force_interactive_options> &options);
  shcore::Value check_instance_state(const Connection_options &instance_def);
  void rescan(const shcore::Option_pack_ref<cluster::Rescan_options> &options);
  void reset_recovery_accounts_password(
      const shcore::Option_pack_ref<Force_interactive_options> &options);
  void force_quorum_using_partition_of(const Connection_options &instance_def,
                                       const char *password = nullptr);
  void disconnect();

  void remove_router_metadata(const std::string &router_def);

  void setup_admin_account(
      const std::string &user,
      const shcore::Option_pack_ref<Setup_account_options> &options);

  void setup_router_account(
      const std::string &user,
      const shcore::Option_pack_ref<Setup_account_options> &options);

  void switch_to_single_primary_mode(
      const Connection_options &instance_def = Connection_options());

  void switch_to_multi_primary_mode(void);
  void set_primary_instance(
      const Connection_options &instance_def,
      const shcore::Option_pack_ref<cluster::Set_primary_instance_options>
          &options = {});

  shcore::Value options(
      const shcore::Option_pack_ref<cluster::Options_options> &options);

  void set_option(const std::string &option, const shcore::Value &value);

  void set_instance_option(const Connection_options &instance_def,
                           const std::string &option,
                           const shcore::Value &value);

  void fence_all_traffic();
  void fence_writes();
  void unfence_writes();

  // ClusterSet
  shcore::Value create_cluster_set(
      const std::string &domain_name,
      const shcore::Option_pack_ref<clusterset::Create_cluster_set_options>
          &options);

  std::shared_ptr<ClusterSet> get_cluster_set();

 protected:
  // Used shell options
  void init();

 private:
  std::shared_ptr<Cluster_impl> m_impl;
  bool m_invalidated = false;

  template <typename TCallback>
  auto execute_with_pool(TCallback &&f, bool interactive) {
    // Invalidate the cached metadata state
    impl()->get_metadata_storage()->invalidate_cached();

    // Init the connection pool
    Scoped_instance_pool ipool(
        interactive,
        Instance_pool::Auth_options(impl()->default_admin_credentials()));

    return f();
  }
};

}  // namespace dba
}  // namespace mysqlsh

#endif  // MODULES_ADMINAPI_MOD_DBA_CLUSTER_H_
