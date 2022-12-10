//@ {VER(>=8.0.27)}

//@<> INCLUDE clusterset_utils.inc

//@<> Setup
testutil.deployRawSandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deployRawSandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host:hostname});

shell.options.useWizards = false;

//@<> configureInstances
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri1, {clusterAdmin:"admin", clusterAdminPassword:"bla"}); });
EXPECT_NO_THROWS(function() { dba.configureInstance(__sandbox_uri2, {clusterAdmin:"admin", clusterAdminPassword:"bla"}); });

testutil.restartSandbox(__mysql_sandbox_port1);
testutil.restartSandbox(__mysql_sandbox_port2);

//@<> create Primary Cluster
shell.connect(__sandbox_uri1);

var cluster;
EXPECT_NO_THROWS(function() { cluster = dba.createCluster("cluster"); });

//@<> createClusterSet
EXPECT_NO_THROWS(function() {cluster.createClusterSet("clusterset"); });

//@<> validate primary cluster
CHECK_PRIMARY_CLUSTER([__sandbox_uri1], cluster)

//@<> dba.getClusterSet()
var clusterset;
EXPECT_NO_THROWS(function() {clusterset = dba.getClusterSet(); });
EXPECT_NE(clusterset, null);

//@<> cluster.getClusterSet()
var cs;
EXPECT_NO_THROWS(function() {cs = cluster.getClusterSet(); });
EXPECT_NE(cs, null);

//@<> createReplicaCluster() - incremental recovery
// SRO might be set in the instance so we must ensure the proper handling of it in createReplicaCluster.
var session4 = mysql.getSession(__sandbox_uri2);
session4.runSql("SET PERSIST super_read_only=true");
var replicacluster;
EXPECT_NO_THROWS(function() {replicacluster = clusterset.createReplicaCluster(__sandbox_uri2, "replicacluster", {recoveryMethod: "incremental"}); });

//@<> validate replica cluster - incremental recovery
CHECK_REPLICA_CLUSTER([__sandbox_uri2], cluster, replicacluster);

//@<> create routers
var clusterset_id = session.runSql("SELECT clusterset_id FROM mysql_innodb_cluster_metadata.clustersets").fetchOne()[0];
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (1, 'system', 'mysqlrouter', 'routerhost1', '8.0.27', '2021-01-01 11:22:33', '{\"bootstrapTargetType\": \"clusterset\", \"ROEndpoint\": \"6481\", \"RWEndpoint\": \"6480\", \"ROXEndpoint\": \"6483\", \"RWXEndpoint\": \"6482\"}', NULL, NULL, ?)", [clusterset_id]);
var cm_router = "routerhost1::system";
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (2, 'system', 'mysqlrouter', 'routerhost2', '8.0.27', '2021-01-01 11:22:33', '{\"bootstrapTargetType\": \"clusterset\", \"ROEndpoint\": \"mysqlro.sock\", \"RWEndpoint\": \"mysql.sock\", \"ROXEndpoint\": \"mysqlxro.sock\", \"RWXEndpoint\": \"mysqlx.sock\"}', NULL, NULL, ?)", [clusterset_id]);
var cr_router = "routerhost2::system";
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (3, 'another', 'mysqlrouter', 'routerhost2', '8.0.27', '2021-01-01 11:22:33', '{\"bootstrapTargetType\": \"clusterset\"}', NULL, NULL, ?)", [clusterset_id]);
var cr_router2 = "routerhost2::another";
session.runSql("INSERT mysql_innodb_cluster_metadata.routers VALUES (4, '', 'mysqlrouter', 'routerhost2', '8.0.27', '2021-01-01 11:22:33', '{\"bootstrapTargetType\": \"clusterset\"}', NULL, NULL, ?)", [clusterset_id]);
var cr_router3 = "routerhost2::";

//@<> clusterset.routingOptions on invalid router
EXPECT_THROWS(function(){ clusterset.routingOptions("invalid_router"); }, "Router 'invalid_router' is not registered in the ClusterSet");

//@ clusterset.routingOptions() with all defaults
values = clusterset.routingOptions();

//@ clusterset.setRoutingOption for a router, all valid values
clusterset.setRoutingOption(cm_router, "target_cluster", "primary");
clusterset.routingOptions(cm_router);
clusterset.setRoutingOption(cm_router, "target_cluster", cluster.getName());
clusterset.routingOptions(cm_router);
// BUG#33298735 clusterset: inconsistent case in/sensitivity in clustername
// Verify combinations of the same name using lower and uppercase to test that the command accepts it
clusterset.setRoutingOption(cm_router, "target_cluster", "Cluster");
clusterset.routingOptions(cm_router);
clusterset.setRoutingOption(cm_router, "target_cluster", "clusteR");
clusterset.routingOptions(cm_router);
clusterset.setRoutingOption(cm_router, "target_cluster", "CLUSTER");
clusterset.routingOptions(cm_router);

clusterset.setRoutingOption(cm_router, 'invalidated_cluster_policy', 'drop_all');
clusterset.routingOptions(cm_router);
clusterset.setRoutingOption(cm_router, 'invalidated_cluster_policy', 'accept_ro');
clusterset.routingOptions(cm_router);

clusterset.setRoutingOption(cm_router, 'stats_updates_frequency', 1);
clusterset.routingOptions(cm_router);
clusterset.setRoutingOption(cm_router, 'stats_updates_frequency', 15);
clusterset.routingOptions(cm_router);

//@<> clusterset.setRoutingOption for a router, invalid values
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, "target_cluster", 'any_not_supported_value'); },
  "Invalid value for routing option 'target_cluster', accepted values 'primary' or valid cluster name");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, "target_cluster", ''); },
  "Invalid value for routing option 'target_cluster', accepted values 'primary' or valid cluster name");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, "target_cluster", ['primary']); },
  "Invalid value for routing option 'target_cluster', accepted values 'primary' or valid cluster name");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, "target_cluster", {'target':'primary'}); },
  "Invalid value for routing option 'target_cluster', accepted values 'primary' or valid cluster name");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, "target_cluster", 1); },
  "Invalid value for routing option 'target_cluster', accepted values 'primary' or valid cluster name");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'invalidated_cluster_policy', 'any_not_supported_value'); },
  "Invalid value for routing option 'invalidated_cluster_policy', accepted values: 'accept_ro', 'drop_all'");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'invalidated_cluster_policy', ''); },
  "Invalid value for routing option 'invalidated_cluster_policy', accepted values: 'accept_ro', 'drop_all'");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'invalidated_cluster_policy', ['primary']); },
  "Invalid value for routing option 'invalidated_cluster_policy', accepted values: 'accept_ro', 'drop_all'");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'invalidated_cluster_policy', {'target':'primary'}); },
  "Invalid value for routing option 'invalidated_cluster_policy', accepted values: 'accept_ro', 'drop_all'");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'invalidated_cluster_policy', 1); },
  "Invalid value for routing option 'invalidated_cluster_policy', accepted values: 'accept_ro', 'drop_all'");

// stats_updates_frequency
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'stats_updates_frequency', ''); },
  "Invalid value for routing option 'stats_updates_frequency', value is expected to be an integer.");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'stats_updates_frequency', 'foo'); },
  "Invalid value for routing option 'stats_updates_frequency', value is expected to be an integer.");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'stats_updates_frequency', ['1']); },
  "Invalid value for routing option 'stats_updates_frequency', value is expected to be an integer.");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'stats_updates_frequency', {'value':'1'}); },
  "Invalid value for routing option 'stats_updates_frequency', value is expected to be an integer.");
EXPECT_THROWS(function(){ clusterset.setRoutingOption(cm_router, 'stats_updates_frequency', -1); },
  "Invalid value for routing option 'stats_updates_frequency', value is expected to be a positive integer.");

//@<> Router does not belong to the clusterset
EXPECT_THROWS(function(){ clusterset.setRoutingOption("abra", 'invalidated_cluster_policy', 'drop_all'); },
  "Router 'abra' is not part of this ClusterSet");
EXPECT_THROWS(function(){ clusterset.setRoutingOption("abra::cadabra", 'target_cluster', 'primary'); },
  "Router 'abra::cadabra' is not part of this ClusterSet");

//@ Resetting router option value for a single router,
clusterset.setRoutingOption(cm_router, "target_cluster", null);
clusterset.routingOptions(cm_router);
clusterset.routingOptions();

clusterset.setRoutingOption(cm_router, 'invalidated_cluster_policy', null);
clusterset.routingOptions(cm_router);
clusterset.routingOptions();

clusterset.setRoutingOption(cm_router, 'stats_updates_frequency', null);
clusterset.routingOptions(cm_router);
clusterset.routingOptions();

//@ clusterset.setRoutingOption all valid values
clusterset.setRoutingOption("target_cluster", "primary");
clusterset.routingOptions();
clusterset.setRoutingOption("target_cluster", cluster.getName());
clusterset.routingOptions();
clusterset.setRoutingOption('invalidated_cluster_policy', 'drop_all');
clusterset.routingOptions();
clusterset.setRoutingOption('invalidated_cluster_policy', 'accept_ro');
clusterset.routingOptions();
clusterset.setRoutingOption(cm_router, "target_cluster", cluster.getName());
clusterset.routingOptions();
clusterset.setRoutingOption(cr_router, "target_cluster", replicacluster.getName());
clusterset.routingOptions();
clusterset.setRoutingOption(cr_router3, "invalidated_cluster_policy", "accept_ro");
clusterset.routingOptions();
clusterset.setRoutingOption("stats_updates_frequency", 11);
clusterset.routingOptions();
clusterset.setRoutingOption(cr_router3, "stats_updates_frequency", 222);
clusterset.routingOptions();

//@<> check types of clusterset router option values
// Bug#34604612
options = clusterset.routingOptions();
EXPECT_EQ("string", typeof options["global"]["target_cluster"]);
EXPECT_EQ("string", typeof options["global"]["invalidated_cluster_policy"]);
EXPECT_EQ("number", typeof options["global"]["stats_updates_frequency"]);

options = JSON.parse(session.runSql("select router_options from mysql_innodb_cluster_metadata.clustersets").fetchOne()[0]);
EXPECT_EQ("string", typeof options["target_cluster"]);
EXPECT_EQ("string", typeof options["invalidated_cluster_policy"]);
EXPECT_EQ("number", typeof options["stats_updates_frequency"]);


//@<> clusterset.setRoutingOption invalid values
EXPECT_THROWS(function(){ clusterset.setRoutingOption("target_cluster", 'any_not_supported_value'); },
  "Invalid value for routing option 'target_cluster', accepted values 'primary' or valid cluster name");
EXPECT_THROWS(function(){ clusterset.setRoutingOption("invalidated_cluster_policy", 'any_not_supported_value'); },
  "Invalid value for routing option 'invalidated_cluster_policy', accepted values: 'accept_ro', 'drop_all'");

//@ Resetting clusterset routing option
// BUG#34458017: setRoutingOption to null resets all options
// Each option should be reset individually and not all of them
clusterset.setRoutingOption("target_cluster", null);
clusterset.routingOptions();

clusterset.setRoutingOption('invalidated_cluster_policy', null);
clusterset.routingOptions();

clusterset.setRoutingOption('stats_updates_frequency', null);
clusterset.routingOptions();

clusterset.setRoutingOption(cr_router, 'target_cluster', null);
clusterset.routingOptions();

//@ clusterset.listRouters
clusterset.setRoutingOption(cm_router, "target_cluster", cluster.getName());
clusterset.setRoutingOption(cr_router, "target_cluster", replicacluster.getName());
clusterset.listRouters();
clusterset.listRouters(cm_router);
clusterset.listRouters(cr_router);

//@<> clusterset.listRouters on invalid router
EXPECT_THROWS(function(){ clusterset.listRouters("invalid_router"); }, "Router 'invalid_router' is not registered in the ClusterSet");

//@<> cluster.listRouters not available for clusterset members
EXPECT_THROWS(function(){ cluster.listRouters(); }, "Function not available for ClusterSet members");
EXPECT_OUTPUT_CONTAINS("Cluster 'cluster' is a member of ClusterSet 'clusterset', use <ClusterSet>.listRouters() to list the ClusterSet Router instances");

//@<> Primary cluster of the clusterset not available
session.runSql("STOP group_replication");
shell.connect(__sandbox_uri2);
clusterset = dba.getClusterSet();

EXPECT_THROWS(function(){ clusterset.setRoutingOption(cr_router, 'invalidated_cluster_policy', 'drop_all'); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");
EXPECT_THROWS(function(){ clusterset.setRoutingOption('target_cluster', 'primary'); }, "The InnoDB Cluster is part of an InnoDB ClusterSet and has global state of NOT_OK within the ClusterSet. Operation is not possible when in that state");

shell.connect(__sandbox_uri1);
dba.rebootClusterFromCompleteOutage("cluster");
cluster = dba.getCluster();
clusterset = dba.getClusterSet();

//@<> special cases with router names
clusterset.setRoutingOption(cr_router2, "target_cluster", "replicacluster");
clusterset.setRoutingOption(cr_router3, "target_cluster", "cluster");

session.runSql("select * from mysql_innodb_cluster_metadata.routers");

EXPECT_EQ(replicacluster.status({extended:1})["defaultReplicaSet"]["groupName"], session.runSql("select options->>'$.target_cluster' from mysql_innodb_cluster_metadata.routers where router_id=3").fetchOne()[0]);
EXPECT_EQ(cluster.status({extended:1})["defaultReplicaSet"]["groupName"], session.runSql("select options->>'$.target_cluster' from mysql_innodb_cluster_metadata.routers where router_id=4").fetchOne()[0]);

clusterset.setRoutingOption(cr_router3, "target_cluster", "replicacluster");
clusterset.setRoutingOption(cr_router2, "target_cluster", "cluster");

EXPECT_EQ(replicacluster.status({extended:1})["defaultReplicaSet"]["groupName"], session.runSql("select options->>'$.target_cluster' from mysql_innodb_cluster_metadata.routers where router_id=4").fetchOne()[0]);
EXPECT_EQ(cluster.status({extended:1})["defaultReplicaSet"]["groupName"], session.runSql("select options->>'$.target_cluster' from mysql_innodb_cluster_metadata.routers where router_id=3").fetchOne()[0]);

//@<> setRoutingOption() after changing the primary instance of a clusterset cluster
cluster.addInstance(__sandbox_uri3, {recoveryMethod:"clone"});
cluster.setPrimaryInstance(__sandbox_uri3);

EXPECT_NO_THROWS(function() { clusterset.setRoutingOption(cr_router3, "target_cluster", "primary"); });

//@<> setRoutingOption() when Shell's active session is not established a primary member
shell.connect(__sandbox_uri1);
cluster = dba.getCluster()
clusterset = dba.getClusterSet()

EXPECT_NO_THROWS(function() { clusterset.setRoutingOption(cr_router3, "target_cluster", "replicacluster"); });

//BUG#33250212 Warn about Routers requiring a re-bootstrap in ClusterSets

//<> listRouters() must print a warning for routers needing a re-bootstrap
shell.connect(__sandbox_uri3);
session.runSql("UPDATE mysql_innodb_cluster_metadata.routers SET attributes = JSON_SET(attributes, '$.bootstrapTargetType', 'cluster') WHERE router_id=2");
session.runSql("UPDATE mysql_innodb_cluster_metadata.routers SET attributes = JSON_SET(attributes, '$.bootstrapTargetType', 'cluster') WHERE router_id=3");

// 'routerhost2::' and 'routerhost2::another' must be displayed in the warning msg

//@<> clusterset.listRouters() warning re-bootstrap
cs.listRouters();
EXPECT_OUTPUT_CONTAINS("WARNING: The following Routers were bootstrapped before the ClusterSet was created: [routerhost2::system, routerhost2::another]. Please re-bootstrap the Routers to ensure the optimal configurations are set.");

//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
