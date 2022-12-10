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

#include "modules/adminapi/cluster/remove_instance.h"

#include <mysqld_error.h>
#include <utility>
#include <vector>

#include "modules/adminapi/common/async_topology.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/errors.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/provision.h"
#include "modules/adminapi/common/sql.h"
#include "modules/adminapi/common/validations.h"
#include "mysqlshdk/libs/config/config.h"
#include "mysqlshdk/libs/config/config_server_handler.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/group_replication.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/shellcore/shell_console.h"

namespace mysqlsh {
namespace dba {
namespace cluster {

Remove_instance::Remove_instance(
    const mysqlshdk::db::Connection_options &instance_cnx_opts,
    const Remove_instance_options &options, Cluster_impl *cluster)
    : m_instance_cnx_opts(instance_cnx_opts),
      m_options(options),
      m_cluster(cluster) {
  assert(cluster);
  m_instance_address =
      m_instance_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());
}

Remove_instance::~Remove_instance() = default;

void Remove_instance::validate_metadata_for_address(
    const std::string &address, Instance_metadata *out_metadata) {
  *out_metadata =
      m_cluster->get_metadata_storage()->get_instance_by_address(address);
}

Instance_metadata Remove_instance::lookup_metadata_for_uuid(
    const std::string &uuid) {
  return m_cluster->get_metadata_storage()->get_instance_by_uuid(uuid);
}

void Remove_instance::ensure_not_last_instance_in_cluster(
    const std::string &removed_uuid) {
  // Check if it is the last instance in the Cluster and issue an error.
  // NOTE: When multiple clusters are supported this check needs to be moved
  //       to a higher level (to check if the instance is the last one of the
  //       last cluster, which should be the default cluster).
  log_debug("Checking if the instance is the last in the cluster");
  if (m_cluster->get_metadata_storage()->get_cluster_size(
          m_cluster->get_id()) == 1) {
    mysqlsh::current_console()->print_error(
        "The instance '" + m_instance_address +
        "' cannot be removed because it is the only member of the Cluster. "
        "Please use <Cluster>.<<<dissolve>>>" +
        "() instead to remove the last instance and dissolve the Cluster.");

    std::string err_msg = "The instance '" + m_instance_address +
                          "' is the last member of the cluster";
    throw shcore::Exception::runtime_error(err_msg);
  }

  log_debug("Checking if the instance is the last ONLINE one in the cluster");
  auto online_instances = m_cluster->get_active_instances(true);
  if (online_instances.size() == 1 &&
      online_instances[0].uuid == removed_uuid) {
    mysqlsh::current_console()->print_error(
        "The instance '" + m_instance_address +
        "' cannot be removed because it is the only ONLINE member of the "
        "Cluster. "
        "Please use <Cluster>.<<<dissolve>>>" +
        "() instead to remove the last instance and dissolve the Cluster.");

    std::string err_msg = "The instance '" + m_instance_address +
                          "' is the last member of the cluster";
    throw shcore::Exception::runtime_error(err_msg);
  }
}

Instance_metadata Remove_instance::remove_instance_metadata() {
  // Save the instance row details (definition). This is required to be able
  // to revert the removal of the instance from the metadata if needed.
  log_debug("Saving instance definition");
  Instance_metadata instance_def =
      m_cluster->get_metadata_storage()->get_instance_by_address(
          m_address_in_metadata);

  log_debug("Removing instance %s from metadata",
            m_address_in_metadata.c_str());
  m_cluster->get_metadata_storage()->remove_instance(m_address_in_metadata);

  return instance_def;
}

void Remove_instance::undo_remove_instance_metadata(
    const Instance_metadata &instance_def) {
  log_debug("Reverting instance removal from metadata");
  m_cluster->get_metadata_storage()->insert_instance(instance_def);

  // Recreate the recovery account previously removed too
  m_cluster->recreate_replication_user(m_target_instance);
}

bool Remove_instance::prompt_to_force_remove() {
  auto console = mysqlsh::current_console();

  console->print_info();
  if (console->confirm("Do you want to continue anyway (only the instance "
                       "metadata will be removed)?",
                       Prompt_answer::NO) == Prompt_answer::YES) {
    console->print_info();
    return true;
  }
  return false;
}

void Remove_instance::check_protocol_upgrade_possible() {
  // Get the instance server_uuid
  mysqlshdk::utils::nullable<std::string> server_uuid;
  std::shared_ptr<Instance> group_instance = m_cluster->get_cluster_server();

  // Determine which instance shall be used to determine if an upgrade is
  // possible and afterwards to perform the upgrade.
  // If the target_instance is leaving the group and is the same as the group
  // session, we must use another instance
  if (m_target_instance) {
    server_uuid = m_target_instance->get_uuid();
  }

  std::string group_instance_uuid = group_instance->get_uuid();

  if (server_uuid == group_instance_uuid) {
    group_instance = m_cluster->get_online_instance(group_instance_uuid);

    if (group_instance) {
      log_info(
          "Using cluster member '%s' for Group Replication protocol upgrade.",
          group_instance->descr().c_str());
    }
  }

  auto console = mysqlsh::current_console();
  try {
    mysqlshdk::utils::Version version;

    // Get the current protocol version in use by the group
    mysqlshdk::utils::Version protocol_version_group =
        mysqlshdk::gr::get_group_protocol_version(*group_instance);

    if (mysqlshdk::gr::is_protocol_upgrade_possible(
            *group_instance, server_uuid.get_safe(""), &version)) {
      console->print_note(
          "The communication protocol used by Group Replication can be "
          "upgraded to version " +
          version.get_full() + ".");

      std::string str;
      if (protocol_version_group < mysqlshdk::utils::Version(8, 0, 16)) {
        str += "Message fragmentation for large transactions";
        if (version == mysqlshdk::utils::Version(8, 0, 27)) {
          str += " and Single Consensus Leader";
        }

        str += " can only be enabled after the protocol is upgraded. ";
      } else {
        str +=
            " Single Consensus Leader can only be enabled after the protocol "
            "is upgraded. ";
      }

      str +=
          "Use Cluster.rescan({upgradeCommProtocol:true}) to perform the "
          "upgrade.";

      console->print_info(str);
    }
  } catch (const shcore::Exception &error) {
    console->print_note(
        "Unable to determine the Group Replication protocol version, while "
        "verifying if a protocol upgrade would be possible: " +
        error.format());
  }
}

void Remove_instance::cleanup_leftover_recovery_account() {
  // Generate the recovery account username that was created for the instance
  std::string user = m_cluster->make_replication_user_name(
      m_instance_id, mysqlshdk::gr::k_group_recovery_user_prefix);

  // Get the Cluster's allowedHost
  std::string host = "%";

  if (shcore::Value allowed_host;
      m_cluster->get_metadata_storage()->query_cluster_attribute(
          m_cluster->get_id(), k_cluster_attribute_replication_allowed_host,
          &allowed_host) &&
      allowed_host.type == shcore::String &&
      !allowed_host.as_string().empty()) {
    host = allowed_host.as_string();
  }

  // The account must not be in use by any other member of the cluster
  if (m_cluster->get_metadata_storage()->count_recovery_account_uses(user) ==
      0) {
    log_info("Dropping recovery account '%s'@'%s' for instance '%s'",
             user.c_str(), host.c_str(), m_instance_address.c_str());

    try {
      m_cluster->get_primary_master()->drop_user(user, host, true);
    } catch (...) {
      mysqlsh::current_console()->print_warning(shcore::str_format(
          "Error dropping recovery account '%s'@'%s' for instance '%s': "
          "%s",
          user.c_str(), host.c_str(), m_instance_address.c_str(),
          format_active_exception().c_str()));
    }
  } else {
    log_info(
        "Recovery account '%s'@'%s' for instance '%s' still in use in "
        "the Cluster: Skipping its removal",
        user.c_str(), host.c_str(), m_instance_address.c_str());
  }
}

void Remove_instance::prepare() {
  auto console = mysqlsh::current_console();
  const auto force = m_options.get_force();

  // Use default port if not provided in the connection options.
  if (!m_instance_cnx_opts.has_port()) {
    m_instance_cnx_opts.set_port(mysqlshdk::db::k_default_mysql_port);
    m_instance_address = m_instance_cnx_opts.as_uri(
        mysqlshdk::db::uri::formats::only_transport());
  }

  // Get instance login information from the cluster session if missing.
  if (!m_instance_cnx_opts.has_user() || !m_instance_cnx_opts.has_password()) {
    std::shared_ptr<Instance> cluster_instance =
        m_cluster->get_cluster_server();
    Connection_options cluster_cnx_opt =
        cluster_instance->get_connection_options();

    if (!m_instance_cnx_opts.has_user() && cluster_cnx_opt.has_user())
      m_instance_cnx_opts.set_user(cluster_cnx_opt.get_user());

    if (!m_instance_cnx_opts.has_password() && cluster_cnx_opt.has_password())
      m_instance_cnx_opts.set_password(cluster_cnx_opt.get_password());
  }

  // Make sure the target instance is not set if an connection error occurs.
  m_target_instance.reset();

  // Try to establish a connection to the target instance, although it might
  // fail if the instance is down.
  // NOTE: Connection required to perform some last validations if the target
  //       instance is available.
  log_debug("Connecting to instance '%s'", m_instance_address.c_str());
  try {
    m_target_instance = Instance::connect(m_instance_cnx_opts);
    log_debug("Successfully connected to instance");

    // If we can connect directly to the instance, the following cases are
    // possible:
    // a - the instance does not belong to the cluster
    // b - the instance belongs to the cluster and the given address matches
    // what's in the MD
    // c - the instance belongs to the cluster, the given address does NOT match
    // what's in the MD, but report_host does
    // d - the instance belongs to the cluster, the given address does NOT match
    // what's in the MD and neither does report_host

    // - a should report the error about it not belonging to the cluster
    // - b should succeed
    // - c should succeed
    // - d shouldn't happen, but is possible. we error out and require the user
    // to pass exactly what's in the MD as the target address, so that it can be
    // removed
  } catch (const shcore::Exception &err) {
    log_warning("Failed to connect to %s: %s",
                m_instance_cnx_opts.uri_endpoint().c_str(), err.what());

    // if the error is a server side error, then it means are able to connect to
    // it, so we just bubble it up to the user (unless force:1)
    if (!mysqlshdk::db::is_mysql_client_error(err.code()) && !force) {
      try {
        throw;
      }
      CATCH_REPORT_AND_THROW_CONNECTION_ERROR(
          m_instance_cnx_opts.uri_endpoint())
    }

    // We can't connect directly to the instance. At this point, the
    // following cases are possible:
    // a - the instance is bogus and doesn't belong to the cluster
    // b - the instance is part of the cluster, but the given address is not
    // what's in the MD
    // c - the instance is part of the cluster and the given address matches
    // what's in the MD
    //
    // interactive:false, force:false:
    // - a should fail with a connection error; because we can't assume
    // report_host isn't in the MD either
    // - b should fail with a connection error; same as above
    // - c should fail with a connection error and suggest using force; we know
    // the instance is valid, but we can't do a safe cleanup, so we need force
    //
    // interactive:true, force:false:
    // - a should fail with a connection error; because we can't assume
    // report_host isn't in the MD and we don't know what to do on force
    // - b should fail with a connection error; same as above
    // - c should fail if force is not given, otherwise prompt whether to force
    // removing the instance although it's not safe
    //
    // interactive:any, force:true:
    // - a should fail with instance doesn't belong to cluster; if forced, then
    // the given address MUST match what's in the MD
    // - b should fail with instance doesn't belong to cluster; same as above
    // - c should succeed

    if (force)
      console->print_note(err.format());
    else
      console->print_warning(err.format());

    try {
      Instance_metadata md;
      validate_metadata_for_address(m_instance_address, &md);
      m_instance_uuid = md.uuid;
      m_instance_id = md.server_id;
      m_address_in_metadata = m_instance_address;
    } catch (const std::exception &e) {
      log_warning("Couldn't get metadata for %s: %s",
                  m_instance_address.c_str(), e.what());

      // If the instance is not in the MD and we can't connect to it either,
      // then there's nothing wen can do with it, force or no force.
      console->print_error("The instance " + m_instance_address +
                           " is not reachable and does not belong to the "
                           "cluster either. Please ensure the member is "
                           "either connectable or remove it through the exact "
                           "address as shown in the cluster status output.");
      throw;
    }

    if (!force) {
      // the address is valid, but we can only remove if force:true
      console->print_error(
          "The instance '" + m_instance_address +
          "' is not reachable and cannot be safely removed from the cluster.");
      console->print_info(
          "To safely remove the instance from the cluster, make sure the "
          "instance is back ONLINE and try again. If you are sure the instance "
          "is permanently unable to rejoin the group and no longer "
          "connectable, use the 'force' option to remove it from the "
          "metadata.");

      if (!m_options.interactive() || m_options.force ||
          !prompt_to_force_remove())
        throw;
    } else {
      // if we're here, we can't connect to the instance but we'll force remove
      // it
      console->print_note(
          "The instance '" + m_instance_address +
          "' is not reachable and it will only be removed from the metadata. "
          "Please take any necessary actions to ensure the instance "
          "will not rejoin the cluster if brought back online.");
      console->print_info();
    }
  }

  // The instance is reachable, we need to make sure that the given address
  // is in the MD and if not, try the same with report_host.
  if (m_target_instance) {
    // First check the address given by the user.
    try {
      Instance_metadata md;
      validate_metadata_for_address(m_instance_address, &md);
      m_instance_uuid = md.uuid;
      m_instance_id = md.server_id;
      m_address_in_metadata = m_instance_address;
    } catch (const shcore::Exception &e) {
      if (e.code() != SHERR_DBA_MEMBER_METADATA_MISSING) throw;

      std::string uuid = m_target_instance->get_uuid();

      // If that's not right, then query it from the instance using the UUID.
      // We do this after checking the address given by the user in case
      // there's some mismatch between the actual UUID and the UUID in the
      // metadata, which for example, could be caused by user restoring or
      // migrating the server. We must assume the user wants to remove whatever
      // they gave as a param to the command, not necessarily whatever the UUID
      // of the server maps to in the metadata.
      try {
        auto md = lookup_metadata_for_uuid(uuid);
        m_instance_uuid = md.uuid;
        m_instance_id = md.server_id;
        m_address_in_metadata = md.address;

        log_debug(
            "Given instance address '%s' was not in metadata, but found a "
            "match for UUID '%s'.",
            m_instance_address.c_str(), uuid.c_str());
      } catch (const shcore::Exception &ee) {
        if (ee.code() != SHERR_DBA_MEMBER_METADATA_MISSING) throw;

        log_debug(
            "Neither the given instance address '%s' nor "
            "@@server_uuid '%s' was found in metadata.",
            m_instance_address.c_str(), uuid.c_str());

        console->print_error("The instance " + m_instance_address +
                             " does not belong to the cluster.");
        // we throw the exception from the 1st lookup, not the 2nd one
      }
      if (m_address_in_metadata.empty()) throw;
    }

    // check the GR state
    mysqlshdk::gr::Member_state member_state =
        mysqlshdk::gr::get_member_state(*m_target_instance);

    if (member_state != mysqlshdk::gr::Member_state::ONLINE &&
        member_state != mysqlshdk::gr::Member_state::RECOVERING) {
      // If not ONLINE, then we can't sync
      // If we're RECOVERING, it could be that the instance will switch to
      // ONLINE and be able to sync, or it could be that there's an applier
      // error or something and the sync will never finish. Since we have a
      // timeout anyway, we'll opt for the potentially useless wait.
      m_skip_sync = true;
      if (force)
        console->print_note(m_target_instance->descr() +
                            " is reachable but has state " +
                            to_string(member_state));
      else
        console->print_error(m_target_instance->descr() +
                             " is reachable but has state " +
                             to_string(member_state));

      // If force:true, then all is fine.
      // If force:false, then throw an error.
      // If force not given, then ask the user if interactive, otherwise error.
      if (!force) {
        console->print_info(
            "To safely remove it from the cluster, it must be brought back "
            "ONLINE. If not possible, use the 'force' option to remove it "
            "anyway.");
        if (!m_options.interactive() || m_options.force ||
            !prompt_to_force_remove())
          throw shcore::Exception(
              "Instance is not ONLINE and cannot be safely removed",
              SHERR_DBA_GROUP_MEMBER_NOT_ONLINE);
      }
    }
  }

  // Ensure instance is not the last in the cluster.
  // Should be called after we know there's any chance the instance actually
  // belongs to the cluster.
  ensure_not_last_instance_in_cluster(m_instance_uuid);

  // Validate user privileges to use the command (only if the instance is
  // available).
  if (m_target_instance) {
    ensure_user_privileges(*m_target_instance);
  }

  check_protocol_upgrade_possible();
}

shcore::Value Remove_instance::execute() {
  auto console = mysqlsh::current_console();

  console->print_para(
      "The instance will be removed from the InnoDB cluster. Depending on the "
      "instance being the Seed or not, the Metadata session might become "
      "invalid. If so, please start a new session to the Metadata Storage R/W "
      "instance.");

  // If this is a replica in a clusterset, ensure view change GTIDs are
  // replicated to the PRIMARY. This is so that view changes GTIDs left in the
  // removed instance don't look like errant transactions if they're added to
  // a different replica cluster
  if (m_cluster->is_cluster_set_member() && !m_cluster->is_primary_cluster()) {
    auto cs = m_cluster->get_cluster_set_object();

    cs->reconcile_view_change_gtids(m_cluster->get_cluster_server().get());
  }

  // Remove recovery user being used by the target instance from the
  // primary. NOTE: This operation MUST be performed before leave-cluster
  // to ensure the that user removal is also propagated to the target
  // instance to remove (if ONLINE), but before metadata removal, since
  // we need account info stored there.
  m_cluster->drop_replication_user(m_target_instance.get(), m_instance_address,
                                   m_instance_uuid, m_instance_id);

  // JOB: Remove instance from the MD (metadata).
  // NOTE: This operation MUST be performed before leave-cluster to ensure
  // that the MD change is also propagated to the target instance to
  // remove (if ONLINE). This avoid issues removing and adding an instance
  // again (error adding the instance because it is already in the MD).
  Instance_metadata instance_def = remove_instance_metadata();
  // Only try to sync transactions with cluster and execute leave-cluster
  // if connection to the instance was previously established (however instance
  // might still fail during the execution of those steps).
  if (m_target_instance) {
    if (m_skip_sync) {
      console->print_note("Transaction sync was skipped");
    } else {
      try {
        // JOB: Sync transactions with the cluster.
        console->print_info("* Waiting for instance '" +
                            m_target_instance->descr() +
                            "' to synchronize with the primary...");
        m_cluster->sync_transactions(
            *m_target_instance, mysqlshdk::gr::k_gr_applier_channel,
            current_shell_options()->get().dba_gtid_wait_timeout);
      } catch (const std::exception &err) {
        // Skip error if force=true, otherwise revert instance remove from MD
        // and issue error.
        // If force is not used OR is set to false
        if (!m_options.force.value_or(false)) {
          // REVERT JOB: Remove instance from the MD (metadata).
          undo_remove_instance_metadata(instance_def);
          // TODO(alfredo): all these checks for auto-rejoin should be done in
          // prepare(), not here
          bool is_rejoining =
              mysqlshdk::gr::is_running_gr_auto_rejoin(*m_target_instance);
          if (is_rejoining)
            console->print_error(
                "The instance '" + m_instance_address +
                "' was unable to catch up with cluster "
                "transactions because auto-rejoin is in progress. It's "
                "possible to use 'force' option to force the removal of it, "
                "but that can leave the instance in an inconsistent state and "
                "lead to errors if you want to reuse it later. It's "
                "recommended to wait for the auto-rejoin process to terminate "
                "and retry the remove operation.");
          else
            console->print_error(
                "The instance '" + m_instance_address +
                "' was unable to catch up with cluster transactions. There "
                "might be too many transactions to apply or some replication "
                "error. In the former case, you can retry the operation (using "
                "a higher timeout value by setting the global shell option "
                "'dba.gtidWaitTimeout'). In the later case, analyze and fix "
                "any replication error. You can also choose to skip this error "
                "using the 'force: true' option, but it might leave the "
                "instance in an inconsistent state and lead to errors if you "
                "want to reuse it.");
          throw;
        } else {
          log_error(
              "Instance '%s' was unable to catch up with cluster transactions: "
              "%s",
              m_instance_address.c_str(), err.what());
          console->print_warning(
              "An error occurred when trying to catch up with cluster "
              "transactions and the instance might have been left in an "
              "inconsistent state that will lead to errors if it is reused.");
          console->print_info();
        }
      }
    }

    // If the instance belongs to a ClusterSet:
    //   - Sync the transactions after dropping the replication user and
    //   updating the metadata
    //   - Reset the ClusterSet replication channel if the cluster is a replica
    //   cluster
    //   - Reset ClusterSet configurations and member actions (after stopping
    //   GR)
    if (m_cluster->is_cluster_set_member()) {
      console->print_info(
          "* Waiting for the Cluster to synchronize with the PRIMARY "
          "Cluster...");

      m_cluster->sync_transactions(*m_cluster->get_primary_master(),
                                   k_clusterset_async_channel_name, 0);

      if (!m_cluster->is_primary_cluster()) {
        // Reset the clusterset replication channel
        remove_channel(m_target_instance.get(), k_clusterset_async_channel_name,
                       false);
      }
    }

    // JOB: Remove the instance from the cluster (Stop GR)
    // NOTE: We always try (best effort) to execute leave_cluster(), but
    // ignore any error if force is used (even if the instance is not
    // reachable, since it is already expected to fail).
    try {
      // Stop Group Replication and reset (persist) GR variables.
      // Also, reset ClusterSet member actions
      mysqlsh::dba::leave_cluster(*m_target_instance,
                                  m_cluster->is_cluster_set_member());

      log_info("Instance '%s' has left the group.", m_instance_address.c_str());
    } catch (const std::exception &err) {
      console->print_error(
          shcore::str_format("Instance '%s' failed to leave the cluster: %s",
                             m_instance_address.c_str(), err.what()));
      // Only add the metadata back if the force option was not used.
      if (!m_options.force.value_or(false)) {
        // REVERT JOB: Remove instance from the MD (metadata).
        // If the removal of the instance from the cluster failed
        // We must add it back to the MD if force is not used
        undo_remove_instance_metadata(instance_def);
        throw;
      }
      // If force is used do not add the instance back to the metadata,
      // and ignore any leave-cluster error.
    }
  }

  // Perform remaining tasks even if the removed instance can't be reached

  // TODO(alfredo) - when the instance being removed is the PRIMARY and
  // we're connected to it, the create_config_object() is effectively a
  // no-op, because it will see all other members are MISSING. This has to
  // be be fixed. This check will accomplish the same thing as before and
  // will avoid the new exception that would be thrown in
  // create_config_object() when it tries to query members from an instance
  // that's not in the group anymore.
  if (!m_target_instance || m_target_instance->get_uuid() !=
                                m_cluster->get_cluster_server()->get_uuid()) {
    try {
      // Update the cluster members (group_seed variable) and remove the
      // replication user from the instance being removed from the primary
      // instance.
      m_cluster->update_group_members_for_removed_member(m_instance_uuid);
    } catch (const std::exception &err) {
      console->print_error(
          shcore::str_format("Could not update remaining cluster members "
                             "after removing '%s': %s",
                             m_instance_address.c_str(), err.what()));
      // Only add the metadata back if the force option was not used.
      if (!m_options.force.value_or(false)) {
        // REVERT JOB: Remove instance from the MD (metadata).
        // If the removal of the instance from the cluster failed
        // We must add it back to the MD if force is not used
        undo_remove_instance_metadata(instance_def);
        throw;
      }
      // If force is used do not add the instance back to the metadata,
      // and ignore any leave-cluster error.
    }
  } else {
    // When removing the primary, we must set m_cluster_server to one of the
    // secondaries to ensure acquire_primary() can get the newest primary and
    // update the topology view
    auto cluster_instances = m_cluster->get_instances();

    for (const auto &i : cluster_instances) {
      // skip the target
      if (i.uuid == m_target_instance->get_uuid()) continue;

      try {
        log_info("Opening a new session to the instance: %s",
                 i.endpoint.c_str());
        Scoped_instance instance(
            m_cluster->connect_target_instance(i.endpoint, false, false));

        // Set it as the new m_cluster_server
        m_cluster->set_target_server(instance);

        break;
      } catch (const shcore::Error &e) {
        continue;
      }
    }
  }

  console->print_info();
  console->print_info("The instance '" + m_instance_address +
                      "' was successfully removed from the cluster.");
  console->print_info();

  return shcore::Value();
}

void Remove_instance::rollback() {
  // Do nothing right now, but it might be used in the future when
  // transactional command execution feature will be available.
}

void Remove_instance::finish() {
  // Close the instance session at the end if available.
  if (m_target_instance) {
    m_target_instance->close_session();
  }
}

}  // namespace cluster
}  // namespace dba
}  // namespace mysqlsh
