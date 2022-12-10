/*
 * Copyright (c) 2017, 2022, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "mysqlshdk/include/shellcore/shell_errors.h"

#ifndef MODULES_ADMINAPI_COMMON_DBA_ERRORS_H_
#define MODULES_ADMINAPI_COMMON_DBA_ERRORS_H_

// AdminAPI / InnoDB cluster error codes
#define SHERR_DBA_FIRST 51000

#define SHERR_DBA_TARGET_QUERY_ERROR 51000

#define SHERR_DBA_GROUP_REPLICATION_UNEXPECTED 51001

#define SHERR_DBA_GROUP_REPLICATION_NOT_RUNNING 51002
#define SHERR_DBA_GROUP_MEMBER_NOT_IN_QUORUM 51003
#define SHERR_DBA_GROUP_MEMBER_NOT_ONLINE 51004

#define SHERR_DBA_GROUP_HAS_NO_QUORUM 51011
#define SHERR_DBA_GROUP_HAS_NO_PRIMARY 51014
#define SHERR_DBA_GROUP_UNREACHABLE 51015
#define SHERR_DBA_GROUP_UNAVAILABLE 51016
#define SHERR_DBA_GROUP_SPLIT_BRAIN 51017
#define SHERR_DBA_GROUP_OFFLINE 51018

#define SHERR_DBA_CLUSTER_NOT_REPLICATED 51020

#define SHERR_DBA_METADATA_MISSING 51101
#define SHERR_DBA_METADATA_READ_ONLY 51102

#define SHERR_DBA_METADATA_INFO_MISSING 51103
#define SHERR_DBA_MEMBER_METADATA_MISSING 51104
#define SHERR_DBA_CLUSTER_METADATA_MISSING 51105
#define SHERR_DBA_METADATA_INCONSISTENT 51106
#define SHERR_DBA_METADATA_UNSUPPORTED 51107
#define SHERR_DBA_METADATA_INVALID 51108
#define SHERR_DBA_METADATA_VERSION_OUTDATED 51109

#define SHERR_DBA_PRIMARY_CLUSTER_UNAVAILABLE 51110
#define SHERR_DBA_PRIMARY_CLUSTER_UNDEFINED 51111
#define SHERR_DBA_PRIMARY_CLUSTER_NOT_FOUND 51112

#define SHERR_DBA_PRIMARY_CLUSTER_STILL_AVAILABLE 51116
#define SHERR_DBA_PRIMARY_CLUSTER_STILL_REACHABLE 51117

#define SHERR_DBA_ASYNC_PRIMARY_UNAVAILABLE 51118
#define SHERR_DBA_ASYNC_PRIMARY_UNDEFINED 51119
#define SHERR_DBA_ASYNC_PRIMARY_NOT_FOUND 51120
#define SHERR_DBA_ASYNC_MEMBER_INVALIDATED 51121
#define SHERR_DBA_ASYNC_MEMBER_INCONSISTENT 51122
#define SHERR_DBA_ASYNC_MEMBER_NOT_REPLICATING 51123
#define SHERR_DBA_ASYNC_MEMBER_UNREACHABLE 51124
#define SHERR_DBA_ASYNC_MEMBER_INVALID_STATUS 51125
#define SHERR_DBA_ASYNC_MEMBER_TOPOLOGY_MISSING 51126

#define SHERR_DBA_BAD_ASYNC_PRIMARY_CANDIDATE 51128
#define SHERR_DBA_NO_ASYNC_PRIMARY_CANDIDATES 51129
#define SHERR_DBA_METADATA_SYNC_ERROR 51130

#define SHERR_DBA_REPLICATION_ERROR 51131
#define SHERR_DBA_REPLICATION_INVALID 51132
#define SHERR_DBA_REPLICATION_OFF 51133

#define SHERR_DBA_GROUP_TOPOLOGY_ERROR 51134

#define SHERR_DBA_REPLICATION_START_TIMEOUT 51140
#define SHERR_DBA_REPLICATION_START_ERROR 51141
#define SHERR_DBA_REPLICATION_AUTH_ERROR 51142
#define SHERR_DBA_REPLICATION_CONNECT_ERROR 51143
#define SHERR_DBA_REPLICATION_COORDINATOR_ERROR 51144
#define SHERR_DBA_REPLICATION_APPLIER_ERROR 51145

#define SHERR_DBA_INVALID_SERVER_CONFIGURATION 51150
#define SHERR_DBA_UNSUPPORTED_ASYNC_TOPOLOGY 51151
#define SHERR_DBA_DATA_ERRANT_TRANSACTIONS 51152
#define SHERR_DBA_DATA_RECOVERY_NOT_POSSIBLE 51153
#define SHERR_DBA_CLONE_RECOVERY_FAILED 51154
#define SHERR_DBA_DISTRIBUTED_RECOVERY_FAILED 51155
#define SHERR_DBA_SERVER_RESTART_TIMEOUT 51156
#define SHERR_DBA_GTID_SYNC_TIMEOUT 51157
#define SHERR_DBA_LOCK_TIMEOUT 51159
#define SHERR_DBA_GTID_SYNC_ERROR 51160
#define SHERR_DBA_UNREACHABLE_INSTANCES 51161
#define SHERR_DBA_FAILOVER_ERROR 51162
#define SHERR_DBA_SWITCHOVER_ERROR 51163
#define SHERR_DBA_UNSUPPORTED_ASYNC_CONFIGURATION 51164
#define SHERR_DBA_PROVISIONING_CLONE_DISABLED 51165
#define SHERR_DBA_PROVISIONING_INCREMENTAL_NOT_POSSIBLE 51166
#define SHERR_DBA_PROVISIONING_EXPLICIT_METHOD_REQUIRED 51167
#define SHERR_DBA_PROVISIONING_EXPLICIT_INCREMENTAL_METHOD_REQUIRED 51168
#define SHERR_DBA_EMPTY_LOCAL_ADDRESS 51169
#define SHERR_DBA_UNSUPPORTED_COMMUNICATION_PROTOCOL 51170
#define SHERR_DBA_UNSUPPORTED_CAPABILITY 51171

// Global topology consistency errors
#define SHERR_DBA_GTCE_GROUP_HAS_MULTIPLE_SLAVES 51200
#define SHERR_DBA_GTCE_GROUP_HAS_MULTIPLE_MASTERS 51201
#define SHERR_DBA_GTCE_GROUP_HAS_WRONG_MASTER 51202

// Invalid arguments
#define SHERR_DBA_BADARG_INSTANCE_NOT_MANAGED 51300
#define SHERR_DBA_BADARG_INSTANCE_ALREADY_MANAGED 51301
#define SHERR_DBA_BADARG_INSTANCE_REMOVE_NOT_ALLOWED 51302
#define SHERR_DBA_BADARG_INSTANCE_NOT_SUPPORTED 51303
#define SHERR_DBA_BADARG_INSTANCE_HAS_METADATA 51304
#define SHERR_DBA_BADARG_INSTANCE_MANAGED_IN_CLUSTER 51305
#define SHERR_DBA_BADARG_INSTANCE_MANAGED_IN_REPLICASET 51306
#define SHERR_DBA_BADARG_INSTANCE_NOT_IN_CLUSTER 51310
#define SHERR_DBA_BADARG_INSTANCE_OUTDATED 51311
#define SHERR_DBA_BADARG_VERSION_NOT_SUPPORTED 51312
#define SHERR_DBA_BADARG_INSTANCE_NOT_PRIMARY 51313
#define SHERR_DBA_BADARG_INSTANCE_NOT_ONLINE 51314
#define SHERR_DBA_BADARG_INSTANCE_ALREADY_IN_GR 51315
#define SHERR_DBA_BADARG_INSTANCE_NOT_IN_CLUSTERSET 51316
#define SHERR_DBA_TARGET_ALREADY_PRIMARY 51317

// Clone handling error
#define SHERR_DBA_CLONE_NO_DONORS 51400
#define SHERR_DBA_CLONE_NO_SUPPORT 51401
#define SHERR_DBA_CLONE_DIFF_VERSION 51402
#define SHERR_DBA_CLONE_DIFF_OS 51403
#define SHERR_DBA_CLONE_DIFF_PLATFORM 51404
#define SHERR_DBA_CLONE_CANCELED 51405

// Lock errors
#define SHERR_DBA_LOCK_GET_FAILED 51500
#define SHERR_DBA_LOCK_GET_TIMEOUT 51501

// ClusterSet errors
#define SHERR_DBA_CLUSTER_STATUS_INVALID 51600
#define SHERR_DBA_CLUSTER_ALREADY_IN_CLUSTERSET 51601
#define SHERR_DBA_CLUSTER_MULTI_PRIMARY_MODE_NOT_SUPPORTED 51602
#define SHERR_DBA_CLUSTER_PRIMARY_UNAVAILABLE 51603
#define SHERR_DBA_CLUSTER_UNSUPPORTED_REPLICATION_CHANNEL 51604
#define SHERR_DBA_INVALID_SERVER_UUID 51605
#define SHERR_DBA_INVALID_SERVER_ID 51606
#define SHERR_DBA_CLUSTER_DOES_NOT_BELONG_TO_CLUSTERSET 51607
#define SHERR_DBA_CLUSTER_CANNOT_REMOVE_PRIMARY_CLUSTER 51608
#define SHERR_DBA_CLUSTER_NOT_CONFIGURED_VIEW_UUID 51609
#define SHERR_DBA_CLUSTER_NAME_ALREADY_IN_USE 51610
#define SHERR_DBA_CLUSTER_UNSUPPORTED_SSL_MODE 51611
#define SHERR_DBA_CLUSTER_IS_ACTIVE_ROUTING_TARGET 51612
#define SHERR_DBA_UNAVAILABLE_REPLICA_CLUSTERS 51613
#define SHERR_DBA_CLUSTER_STILL_RESTORABLE 51614
#define SHERR_DBA_CLUSTER_PRIMARY_STILL_AVAILABLE 51615
#define SHERR_DBA_UNSUPPORTED_CLUSTER_TYPE 51616
#define SHERR_DBA_CLUSTER_NOT_FENCED 51617
#define SHERR_DBA_CLUSTER_NOT_CONFIGURED_TRANSACTION_SIZE_LIMIT 51618

#endif  // MODULES_ADMINAPI_COMMON_DBA_ERRORS_H_