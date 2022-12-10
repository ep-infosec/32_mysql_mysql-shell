//@ deploy the sandbox
||

//@ ConfigureLocalInstance should fail if there's no session nor parameters provided
||An open session is required to perform this operation.

//@<OUT> Interactive_dba_configure_local_instance read_only_no_prompts
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
Assuming full account name 'admin'@'%' for admin

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
?{VER(<8.0.3)}
+----------------------------------+---------------+----------------+------------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                           |
+----------------------------------+---------------+----------------+------------------------------------------------+
| binlog_checksum                  | CRC32         | NONE           | Update the server variable and the config file |
| binlog_format                    | <not set>     | ROW            | Update the config file                         |
| enforce_gtid_consistency         | OFF           | ON             | Update the config file and restart the server  |
| gtid_mode                        | OFF           | ON             | Update the config file and restart the server  |
| log_bin                          | <not set>     | <no value>     | Update the config file and restart the server  |
| log_slave_updates                | OFF           | ON             | Update the config file and restart the server  |
| master_info_repository           | FILE          | TABLE          | Update the config file and restart the server  |
| relay_log_info_repository        | FILE          | TABLE          | Update the config file and restart the server  |
| report_port                      | <not set>     | <<<__mysql_sandbox_port1>>>           | Update the server variable and the config file |
| server_id                        | 0             | <unique ID>    | Update the config file and restart the server  |
| transaction_write_set_extraction | OFF           | XXHASH64       | Update the config file and restart the server  |
+----------------------------------+---------------+----------------+------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server: an option file is required.
Cluster admin user 'admin'@'%' created.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
?{}
?{VER(>=8.0.3) && VER(<8.0.21)}
+----------------------------------+---------------+----------------+------------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                           |
+----------------------------------+---------------+----------------+------------------------------------------------+
| binlog_checksum                  | CRC32         | NONE           | Update the server variable and the config file |
| binlog_format                    | <not set>     | ROW            | Update the config file                         |
| enforce_gtid_consistency         | OFF           | ON             | Update the config file and restart the server  |
| gtid_mode                        | OFF           | ON             | Update the config file and restart the server  |
| master_info_repository           | <not set>     | TABLE          | Update the config file                         |
| relay_log_info_repository        | <not set>     | TABLE          | Update the config file                         |
| report_port                      | <not set>     | <<<__mysql_sandbox_port1>>>           | Update the config file                         |
| server_id                        | 1             | <unique ID>    | Update the config file and restart the server  |
| transaction_write_set_extraction | <not set>     | XXHASH64       | Update the config file                         |
+----------------------------------+---------------+----------------+------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Cluster admin user 'admin'@'%' created.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
?{}
?{VER(>=8.0.11) && VER(<8.0.23)}
+----------------------------------+---------------+----------------+-----------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                          |
+----------------------------------+---------------+----------------+-----------------------------------------------+
| binlog_format                    | <not set>     | ROW            | Update the config file                        |
| enforce_gtid_consistency         | OFF           | ON             | Update the config file and restart the server |
| gtid_mode                        | OFF           | ON             | Update the config file and restart the server |
| master_info_repository           | <not set>     | TABLE          | Update the config file                        |
| relay_log_info_repository        | <not set>     | TABLE          | Update the config file                        |
| report_port                      | <not set>     | <<<__mysql_sandbox_port1>>>           | Update the config file                        |
| server_id                        | 1             | <unique ID>    | Update the config file and restart the server |
| transaction_write_set_extraction | <not set>     | XXHASH64       | Update the config file                        |
+----------------------------------+---------------+----------------+-----------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Cluster admin user 'admin'@'%' created.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
?{}
?{VER(>=8.0.23) && VER(<8.0.26)}
+----------------------------------------+---------------+----------------+------------------------------------------------+
| Variable                               | Current Value | Required Value | Note                                           |
+----------------------------------------+---------------+----------------+------------------------------------------------+
| binlog_format                          | <not set>     | ROW            | Update the config file                         |
| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable and the config file |
| enforce_gtid_consistency               | OFF           | ON             | Update the config file and restart the server  |
| gtid_mode                              | OFF           | ON             | Update the config file and restart the server  |
| report_port                            | <not set>     | <<<__mysql_sandbox_port1>>>           | Update the config file                         |
| server_id                              | 1             | <unique ID>    | Update the config file and restart the server  |
| slave_parallel_type                    | DATABASE      | LOGICAL_CLOCK  | Update the server variable and the config file |
| slave_preserve_commit_order            | OFF           | ON             | Update the server variable and the config file |
| transaction_write_set_extraction       | <not set>     | XXHASH64       | Update the config file                         |
+----------------------------------------+---------------+----------------+------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Cluster admin user 'admin'@'%' created.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
?{}
?{VER(==8.0.26)}
+----------------------------------------+---------------+----------------+------------------------------------------------+
| Variable                               | Current Value | Required Value | Note                                           |
+----------------------------------------+---------------+----------------+------------------------------------------------+
| binlog_format                          | <not set>     | ROW            | Update the config file                         |
| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable and the config file |
| enforce_gtid_consistency               | OFF           | ON             | Update the config file and restart the server  |
| gtid_mode                              | OFF           | ON             | Update the config file and restart the server  |
| replica_parallel_type                  | DATABASE      | LOGICAL_CLOCK  | Update the server variable and the config file |
| replica_preserve_commit_order          | OFF           | ON             | Update the server variable and the config file |
| report_port                            | <not set>     | <<<__mysql_sandbox_port1>>>           | Update the config file                         |
| server_id                              | 1             | <unique ID>    | Update the config file and restart the server  |
| transaction_write_set_extraction       | <not set>     | XXHASH64       | Update the config file                         |
+----------------------------------------+---------------+----------------+------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Cluster admin user 'admin'@'%' created.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
?{}
?{VER(>=8.0.27)}
+----------------------------------------+---------------+----------------+------------------------------------------------+
| Variable                               | Current Value | Required Value | Note                                           |
+----------------------------------------+---------------+----------------+------------------------------------------------+
| binlog_format                          | <not set>     | ROW            | Update the config file                         |
| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable and the config file |
| enforce_gtid_consistency               | OFF           | ON             | Update the config file and restart the server  |
| gtid_mode                              | OFF           | ON             | Update the config file and restart the server  |
| replica_parallel_type                  | <not set>     | LOGICAL_CLOCK  | Update the config file                         |
| replica_preserve_commit_order          | <not set>     | ON             | Update the config file                         |
| report_port                            | <not set>     | <<<__mysql_sandbox_port1>>>           | Update the config file                         |
| server_id                              | 1             | <unique ID>    | Update the config file and restart the server  |
| transaction_write_set_extraction       | <not set>     | XXHASH64       | Update the config file                         |
+----------------------------------------+---------------+----------------+------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Cluster admin user 'admin'@'%' created.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
?{}

//@<OUT> Interactive_dba_configure_local_instance read_only_no_flag_prompt_yes 8.0 {VER(>=8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
Assuming full account name 'root2'@'%' for root2

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]:
The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system
variable set to protect it from inadvertent updates from applications.
You must first unset it to be able to perform any changes to this instance.
For more information see:
https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

NOTE: There are open sessions to 'localhost:<<<__mysql_sandbox_port1>>>'.
You may want to kill these sessions to prevent them from performing unexpected updates:

1 open session(s) of 'root@localhost'.

Do you want to disable super_read_only and continue? [y/N]: Disabled super_read_only on the instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'
Cluster admin user 'root2'@'%' created.
Enabling super_read_only on the instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@<OUT> Interactive_dba_configure_local_instance read_only_no_flag_prompt_yes 5.7 {VER(<8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
Assuming full account name 'root2'@'%' for root2

NOTE: Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+--------------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                             |
+----------------------------------+---------------+----------------+--------------------------------------------------+
| enforce_gtid_consistency         | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                        | OFF           | ON             | Update read-only variable and restart the server |
| log_bin                          | OFF           | ON             | Restart the server                               |
| log_slave_updates                | OFF           | ON             | Update read-only variable and restart the server |
| master_info_repository           | FILE          | TABLE          | Update read-only variable and restart the server |
| relay_log_info_repository        | FILE          | TABLE          | Update read-only variable and restart the server |
| server_id                        | 0             | <unique ID>    | Update read-only variable and restart the server |
| transaction_write_set_extraction | OFF           | XXHASH64       | Update read-only variable and restart the server |
+----------------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]:
The MySQL instance at 'localhost:<<<__mysql_sandbox_port1>>>' currently has the super_read_only system
variable set to protect it from inadvertent updates from applications.
You must first unset it to be able to perform any changes to this instance.
For more information see:
https://dev.mysql.com/doc/refman/en/server-system-variables.html#sysvar_super_read_only.

NOTE: There are open sessions to 'localhost:<<<__mysql_sandbox_port1>>>'.
You may want to kill these sessions to prevent them from performing unexpected updates:

1 open session(s) of 'root@localhost'.

Do you want to disable super_read_only and continue? [y/N]: Disabled super_read_only on the instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'
Cluster admin user 'root2'@'%' created.
Enabling super_read_only on the instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'

//@ test configureLocalInstance providing clusterAdminPassword without clusterAdmin
||The clusterAdminPassword is allowed only if clusterAdmin is specified.

//@ test configureLocalInstance providing clusterAdminPassword and an existing clusterAdmin
||The 'root2'@'%' account already exists, clusterAdminPassword is not allowed for an existing account.

//@ Interactive_dba_configure_local_instance read_only_no_flag_prompt_no 8.0 {VER(>=8.0.11)}
||Cancelled (RuntimeError)

//@ Interactive_dba_configure_local_instance read_only_no_flag_prompt_no 5.7 {VER(<8.0.11)}
||Cancelled (RuntimeError)

//@ Interactive_dba_configure_local_instance read_only_invalid_flag_value
||Option 'clearReadOnly' Bool expected, but value is String

//@<OUT> Interactive_dba_configure_local_instance read_only_flag_true 8.0 {VER(>=8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
Assuming full account name 'root5'@'%' for root5

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]: Disabled super_read_only on the instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'
Cluster admin user 'root5'@'%' created.
Enabling super_read_only on the instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'

//@<OUT> Interactive_dba_configure_local_instance read_only_flag_true 5.7 {VER(<8.0.11)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
Assuming full account name 'root5'@'%' for root5

NOTE: Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+--------------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                             |
+----------------------------------+---------------+----------------+--------------------------------------------------+
| enforce_gtid_consistency         | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                        | OFF           | ON             | Update read-only variable and restart the server |
| log_bin                          | OFF           | ON             | Restart the server                               |
| log_slave_updates                | OFF           | ON             | Update read-only variable and restart the server |
| master_info_repository           | FILE          | TABLE          | Update read-only variable and restart the server |
| relay_log_info_repository        | FILE          | TABLE          | Update read-only variable and restart the server |
| server_id                        | 0             | <unique ID>    | Update read-only variable and restart the server |
| transaction_write_set_extraction | OFF           | XXHASH64       | Update read-only variable and restart the server |
+----------------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Disabled super_read_only on the instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'
Cluster admin user 'root5'@'%' created.
Enabling super_read_only on the instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>'

//@ Interactive_dba_configure_local_instance read_only_flag_false 8.0 {VER(>=8.0.11)}
||

//@ Interactive_dba_configure_local_instance read_only_flag_false 5.7 {VER(<8.0.11)}
||

//@ Cleanup raw sandbox
||

//@ deploy sandbox, change dynamic variable values on the configuration and make it read-only (BUG#27702439)
||

//@<OUT> Interactive_dba_configure_local_instance should ask for creation of new configuration file and then ask user to copy it. (BUG#27702439)
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

ERROR: User 'root' can only connect from 'localhost'. New account(s) with proper source address specification to allow remote connection from all instances must be created to manage the cluster.

1) Create remotely usable account for 'root' with same grants and password
2) Create a new admin account for InnoDB cluster with minimal required grants
3) Ignore and continue
4) Cancel

Please select an option [1]: Please provide a source address filter for the account (e.g: 192.168.% or % etc) or leave empty and press Enter to cancel.
Account Host:
?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
+---------------+---------------+----------------+------------------------+
| Variable      | Current Value | Required Value | Note                   |
+---------------+---------------+----------------+------------------------+
| binlog_format | STATEMENT     | ROW            | Update the config file |
+---------------+---------------+----------------+------------------------+

Sandbox MySQL configuration file at: <<<mycnf_path>>>
WARNING: mycnfPath is not writable: <<<mycnf_path>>>: Permission denied
The required configuration changes may be written to a different file, which you can copy over to its proper location.
Output path for updated configuration file: Do you want to perform the required configuration changes? [y/n]:
Cluster admin user 'root'@'%' created.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster but you must copy <<<mycnf_path>>>2 to the MySQL option file path.

//@ Cleanup (BUG#27702439)
||

//@ Deploy raw sandbox BUG#29725222 {VER(>= 8.0.17)}
||

//@<OUT> Run configure and restart instance BUG#29725222 {VER(>= 8.0.17) && VER(< 8.0.21)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

NOTE: Some configuration options need to be fixed:
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+
| binlog_checksum          | CRC32         | NONE           | Update the server variable                       |
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]: Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
Restarting MySQL...
NOTE: MySQL server at <<<hostname>>>:<<<__mysql_sandbox_port1>>> was restarted.

//@<OUT> Run configure and restart instance BUG#29725222 {VER(>= 8.0.21) && VER(< 8.0.23)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
NOTE: Some configuration options need to be fixed:
+--------------------------+---------------+----------------+--------------------------------------------------+
| Variable                 | Current Value | Required Value | Note                                             |
+--------------------------+---------------+----------------+--------------------------------------------------+
| enforce_gtid_consistency | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                | OFF           | ON             | Update read-only variable and restart the server |
| server_id                | 1             | <unique ID>    | Update read-only variable and restart the server |
+--------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]: Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
Restarting MySQL...
NOTE: MySQL server at <<<hostname>>>:<<<__mysql_sandbox_port1>>> was restarted.

//@<OUT> Run configure and restart instance BUG#29725222 {VER(>= 8.0.23) && VER(<8.0.26)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

applierWorkerThreads will be set to the default value of 4.

NOTE: Some configuration options need to be fixed:
+----------------------------------------+---------------+----------------+--------------------------------------------------+
| Variable                               | Current Value | Required Value | Note                                             |
+----------------------------------------+---------------+----------------+--------------------------------------------------+
| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable                       |
| enforce_gtid_consistency               | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                              | OFF           | ON             | Update read-only variable and restart the server |
| server_id                              | 1             | <unique ID>    | Update read-only variable and restart the server |
| slave_parallel_type                    | DATABASE      | LOGICAL_CLOCK  | Update the server variable                       |
| slave_preserve_commit_order            | OFF           | ON             | Update the server variable                       |
+----------------------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]: Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
Restarting MySQL...
NOTE: MySQL server at <<<hostname>>>:<<<__mysql_sandbox_port1>>> was restarted.

//@<OUT> Run configure and restart instance BUG#29725222 {VER(==8.0.26)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

applierWorkerThreads will be set to the default value of 4.

NOTE: Some configuration options need to be fixed:
+----------------------------------------+---------------+----------------+--------------------------------------------------+
| Variable                               | Current Value | Required Value | Note                                             |
+----------------------------------------+---------------+----------------+--------------------------------------------------+
| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable                       |
| enforce_gtid_consistency               | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                              | OFF           | ON             | Update read-only variable and restart the server |
| replica_parallel_type                  | DATABASE      | LOGICAL_CLOCK  | Update the server variable                       |
| replica_preserve_commit_order          | OFF           | ON             | Update the server variable                       |
| server_id                              | 1             | <unique ID>    | Update read-only variable and restart the server |
+----------------------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]: Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
Restarting MySQL...
NOTE: MySQL server at <<<hostname>>>:<<<__mysql_sandbox_port1>>> was restarted.

//@<OUT> Run configure and restart instance BUG#29725222 {VER(>=8.0.27)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

applierWorkerThreads will be set to the default value of 4.

NOTE: Some configuration options need to be fixed:
+----------------------------------------+---------------+----------------+--------------------------------------------------+
| Variable                               | Current Value | Required Value | Note                                             |
+----------------------------------------+---------------+----------------+--------------------------------------------------+
| binlog_transaction_dependency_tracking | COMMIT_ORDER  | WRITESET       | Update the server variable                       |
| enforce_gtid_consistency               | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                              | OFF           | ON             | Update read-only variable and restart the server |
| server_id                              | 1             | <unique ID>    | Update read-only variable and restart the server |
+----------------------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server.
Do you want to perform the required configuration changes? [y/n]: Do you want to restart the instance after configuring it? [y/n]: Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
Restarting MySQL...
NOTE: MySQL server at <<<hostname>>>:<<<__mysql_sandbox_port1>>> was restarted.

//@<OUT> Confirm changes were applied and everything is fine BUG#29725222 {VER(>= 8.0.17)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is valid to be used in an InnoDB cluster.
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' is already ready to be used in an InnoDB cluster.

//@ Cleanup BUG#29725222 {VER(>= 8.0.17)}
||

//@<OUT> canonical IPv6 addresses are supported WL#12758 {VER(>= 8.0.14)}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as [::1]:<<<__mysql_sandbox_port1>>>

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
The instance '[::1]:<<<__mysql_sandbox_port1>>>' is valid to be used in an InnoDB cluster.
The instance '[::1]:<<<__mysql_sandbox_port1>>>' is already ready to be used in an InnoDB cluster.

//@<OUT> canonical IPv4 addresses are supported WL#12758
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...
NOTE: Instance detected as a sandbox.
Please note that sandbox instances are only suitable for deploying test clusters for use within the same host.

This instance reports its own address as 127.0.0.1:<<<__mysql_sandbox_port1>>>

?{VER(>=8.0.23)}
applierWorkerThreads will be set to the default value of 4.

?{}
The instance '127.0.0.1:<<<__mysql_sandbox_port1>>>' is valid to be used in an InnoDB cluster.
The instance '127.0.0.1:<<<__mysql_sandbox_port1>>>' is already ready to be used in an InnoDB cluster.

//@ Deploy raw sandbox, and check that configureLocalInstance is using the config path from interactive prompt (BUG#29554251) {VER(< 8.0.0) && __dbug_off == 0}
||

//@<OUT> Interactive_dba_configure_local_instance where we pass the configuration file path via wizard. (BUG#29554251) {VER(< 8.0.0) && __dbug_off == 0}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

NOTE: Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+------------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                           |
+----------------------------------+---------------+----------------+------------------------------------------------+
| binlog_checksum                  | CRC32         | NONE           | Update the server variable and the config file |
| disable_log_bin                  | <not set>     | <not set>      | Update the config file and restart the server  |
| enforce_gtid_consistency         | OFF           | ON             | Update the config file and restart the server  |
| gtid_mode                        | OFF           | ON             | Update the config file and restart the server  |
| log_bin                          | <not set>     | <no value>     | Update the config file and restart the server  |
| log_slave_updates                | OFF           | ON             | Update the config file and restart the server  |
| master_info_repository           | FILE          | TABLE          | Update the config file and restart the server  |
| relay_log_info_repository        | FILE          | TABLE          | Update the config file and restart the server  |
| server_id                        | 0             | <unique ID>    | Update the config file and restart the server  |
| skip_log_bin                     | <not set>     | <not set>      | Update the config file and restart the server  |
| transaction_write_set_extraction | OFF           | XXHASH64       | Update the config file and restart the server  |
+----------------------------------+---------------+----------------+------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server: an option file is required.

Detecting the configuration file...
Default file not found at the standard locations.
Please specify the path to the MySQL configuration file: Do you want to perform the required configuration changes? [y/n]: Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@<OUT> Confirm binlog_checksum is wrote to config file (BUG#30171090) {VER(< 8.0.0) && __dbug_off == 0}
[
    "binlog_checksum = NONE"
]

//@<OUT> Confirm that changes were applied to config file (BUG#29554251) {VER(< 8.0.0) && __dbug_off == 0}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>

NOTE: Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+--------------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                             |
+----------------------------------+---------------+----------------+--------------------------------------------------+
| binlog_format                    | <not set>     | ROW            | Update the config file                           |
| enforce_gtid_consistency         | OFF           | ON             | Update read-only variable and restart the server |
| gtid_mode                        | OFF           | ON             | Update read-only variable and restart the server |
| log_bin                          | OFF           | ON             | Restart the server                               |
| log_slave_updates                | OFF           | ON             | Update read-only variable and restart the server |
| master_info_repository           | FILE          | TABLE          | Update read-only variable and restart the server |
| relay_log_info_repository        | FILE          | TABLE          | Update read-only variable and restart the server |
| report_port                      | <not set>     | <<<__mysql_sandbox_port1>>>           | Update the config file                           |
| server_id                        | 0             | <unique ID>    | Update read-only variable and restart the server |
| transaction_write_set_extraction | OFF           | XXHASH64       | Update read-only variable and restart the server |
+----------------------------------+---------------+----------------+--------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server: an option file is required.
Do you want to perform the required configuration changes? [y/n]: Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.

//@ Cleanup (BUG#29554251) {VER(< 8.0.0) && __dbug_off == 0}
||

//@<OUT> (BUG#30657204) configure local instance should succeed {VER(< 8.0.0) && __dbug_off == 0}
Configuring local MySQL instance listening at port <<<__mysql_sandbox_port1>>> for use in an InnoDB cluster...

This instance reports its own address as <<<hostname>>>:<<<__mysql_sandbox_port1>>>
Assuming full account name 'admin'@'%' for admin

NOTE: Some configuration options need to be fixed:
+----------------------------------+---------------+----------------+------------------------------------------------+
| Variable                         | Current Value | Required Value | Note                                           |
+----------------------------------+---------------+----------------+------------------------------------------------+
| binlog_checksum                  | CRC32         | NONE           | Update the server variable and the config file |
| disable_log_bin                  | <not set>     | <not set>      | Update the config file and restart the server  |
| enforce_gtid_consistency         | OFF           | ON             | Update the config file and restart the server  |
| gtid_mode                        | OFF           | ON             | Update the config file and restart the server  |
| log_bin                          | <not set>     | <no value>     | Update the config file and restart the server  |
| log_slave_updates                | OFF           | ON             | Update the config file and restart the server  |
| master_info_repository           | FILE          | TABLE          | Update the config file and restart the server  |
| relay_log_info_repository        | FILE          | TABLE          | Update the config file and restart the server  |
| server_id                        | 0             | <unique ID>    | Update the config file and restart the server  |
| skip_log_bin                     | <not set>     | <not set>      | Update the config file and restart the server  |
| transaction_write_set_extraction | OFF           | XXHASH64       | Update the config file and restart the server  |
+----------------------------------+---------------+----------------+------------------------------------------------+

Some variables need to be changed, but cannot be done dynamically on the server: an option file is required.

Detecting the configuration file...
Default file not found at the standard locations.
Please specify the path to the MySQL configuration file: WARNING: mycnfPath is not writable: <<<bug_mycnf_path>>>: Permission denied
The required configuration changes may be written to a different file, which you can copy over to its proper location.
Output path for updated configuration file: Do you want to perform the required configuration changes? [y/n]:
Cluster admin user 'admin'@'%' created.
Configuring instance...
The instance '<<<hostname>>>:<<<__mysql_sandbox_port1>>>' was configured to be used in an InnoDB cluster but you must copy <<<bug_mycnf_path>>>2 to the MySQL option file path.
NOTE: MySQL server needs to be restarted for configuration changes to take effect.
