// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>

//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, 'root', {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port1);
testutil.deploySandbox(__mysql_sandbox_port2, 'root', {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port2);
testutil.deploySandbox(__mysql_sandbox_port3, 'root', {report_host: hostname});
testutil.snapshotSandboxConf(__mysql_sandbox_port3);

shell.connect(__sandbox_uri1);

// Create a test dataset so that RECOVERY takes a while and we ensure right monitoring messages for addInstance
session.runSql("create schema test");
session.runSql("create table test.data (a int primary key auto_increment, data longtext)");
for (i = 0; i < 20; i++) {
    session.runSql("insert into test.data values (default, repeat('x', 4*1024*1024))");
}

var cluster = dba.createCluster('devCluster', {memberSslMode: 'REQUIRED', gtidSetIsComplete: true});

var Cluster = dba.getCluster('devCluster');

// Sets the correct local host
var desc = Cluster.describe();
var localhost = desc.defaultReplicaSet.topology[0].label.split(':')[0];


//@<> Cluster: validating members
validateMembers(Cluster, [
  'name',
  'getClusterSet',
  'getName',
  'addInstance',
  'removeInstance',
  'rejoinInstance',
  'checkInstanceState',
  'describe',
  'status',
  'help',
  'dissolve',
  'disconnect',
  'rescan',
  'listRouters',
  'removeRouterMetadata',
  'resetRecoveryAccountsPassword',
  'forceQuorumUsingPartitionOf',
  'switchToSinglePrimaryMode',
  'switchToMultiPrimaryMode',
  'setPrimaryInstance',
  'options',
  'setOption',
  'setInstanceOption',
  'setupAdminAccount',
  'setupRouterAccount',
  'createClusterSet',
  'fenceAllTraffic',
  'fenceWrites',
  'unfenceWrites'
])

//@ Cluster: addInstance errors
Cluster.addInstance();
Cluster.addInstance(5,6,7,1);
Cluster.addInstance(5, 5);
Cluster.addInstance('', 5);
Cluster.addInstance(5);
Cluster.addInstance('');
Cluster.addInstance({host: "localhost", schema: 'abs', user:"sample", "auth-method":56, memberSslMode: "foo", ipWhitelist: " "});
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, "root");
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, {memberSslMode: "foo", password: "root"});
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, {memberSslMode: "", password: "root"});
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, {ipWhitelist: " ", password: "root"});
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, {label: "", password: "root"});
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, {label: "#invalid", password: "root"});
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, {label: "invalid#char", password: "root"});
Cluster.addInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port2}, {label: "over256chars_1234567890123456789012345678990123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123", password: "root"});

//@ Cluster: addInstance with interaction, error
add_instance_options['port'] = __mysql_sandbox_port1;
add_instance_options['user'] = 'root';
Cluster.addInstance(add_instance_options, add_instance_extra_opts);

//@<OUT> Cluster: addInstance with interaction, ok
Cluster.addInstance(__sandbox_uri2);

//@<OUT> Cluster: addInstance 3 with interaction, ok
Cluster.addInstance(__sandbox_uri3);

//@<OUT> Cluster: describe1
Cluster.describe();

//@<> Cluster: status1
var status = Cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"])


//@ Cluster: removeInstance errors
Cluster.removeInstance();
Cluster.removeInstance(1,2,3);
Cluster.removeInstance(1);
Cluster.removeInstance({host: "localhost", port:33060, schema: 'abs', user:"sample", "auth-method":56});
// try to remove instance that is not in the cluster using the classic port
Cluster.removeInstance({user: __user, host: __host, port: __mysql_port, password: shell.parseUri(__uripwd).password});

//@ Cluster: removeInstance
Cluster.removeInstance({host: "localhost", port:__mysql_sandbox_port2});

//@<OUT> Cluster: describe2
Cluster.describe();

//@<> Cluster: status2
var status = Cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["mode"])

//@<OUT> Cluster: dissolve error: not empty
// WL11889 FR2_01: prompt to confirm dissolve in interactive mode.
// Regression for BUG#27837231: useless 'force' parameter for dissolve
Cluster.dissolve();

//@ Cluster: dissolve errors
Cluster.dissolve(1);
Cluster.dissolve(1,2);
Cluster.dissolve("");
Cluster.dissolve({enforce: true});
Cluster.dissolve({force: 'sample'});

//@ Cluster: remove_instance 3
Cluster.removeInstance({host:localhost, port:__mysql_sandbox_port3});

//@<OUT> Cluster: addInstance with interaction, ok 3
Cluster.addInstance(__sandbox_uri2, {'label': 'z2nd_sandbox'});

//@<OUT> Cluster: addInstance with interaction, ok 4
Cluster.addInstance(__sandbox_uri3, {'label': 'z3rd_sandbox'});

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Cluster: status: success
var status = Cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"]["z2nd_sandbox"]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"]["z3rd_sandbox"]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"]["z2nd_sandbox"]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"]["z3rd_sandbox"]["mode"])

// Rejoin tests

//@# Dba: kill instance 3
testutil.killSandbox(__mysql_sandbox_port3);

testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

//@# Dba: start instance 3
testutil.startSandbox(__mysql_sandbox_port3);

//@: Cluster: rejoinInstance errors
Cluster.rejoinInstance();
Cluster.rejoinInstance(1,2,3);
Cluster.rejoinInstance(1);
Cluster.rejoinInstance({host: "localhost", schema: "abs", "auth-method":56, memberSslMode: "foo", ipWhitelist: " "});
Cluster.rejoinInstance("somehost:3306", "root");
Cluster.rejoinInstance({host: "localhost"});
Cluster.rejoinInstance("localhost:3306");
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3}, {memberSslMode: "foo", password: "root"});
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3}, {memberSslMode: "", password: "root"});
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3}, {ipWhitelist: " ", password: "root"});

//@<OUT> Cluster: rejoinInstance with interaction, ok
var session3 = mysql.getSession(__sandbox_uri3);
var server_id = session3.runSql("select @@server_id").fetchOne()[0];
var repl_user = "mysql_innodb_cluster_"+server_id;
Cluster.rejoinInstance({dbUser: "root", host: "localhost", port: __mysql_sandbox_port3}, {memberSslMode: "AUTO", password: 'root'});
EXPECT_STDOUT_CONTAINS("WARNING: Option 'memberSslMode' is deprecated for this operation and it will be removed in a future release. This option is not needed because the SSL mode is automatically obtained from the cluster. Please do not use it here.")
EXPECT_STDOUT_CONTAINS("WARNING: 'dbUser' connection option is deprecated, use 'user' option instead.");

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Verify if the cluster is OK

//@<> Cluster: status for rejoin: success
var status = Cluster.status();
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"]["z2nd_sandbox"]["status"])
EXPECT_EQ("ONLINE", status["defaultReplicaSet"]["topology"]["z3rd_sandbox"]["status"])
EXPECT_EQ("R/W", status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"]["z2nd_sandbox"]["mode"])
EXPECT_EQ("R/O", status["defaultReplicaSet"]["topology"]["z3rd_sandbox"]["mode"])

//@<OUT> Cluster: final dissolve
// WL11889 FR2_01: prompt to confirm dissolve in interactive mode.
// WL11889 FR2_02: force option no longer required.
// Regression for BUG#27837231: useless 'force' parameter for dissolve
Cluster.dissolve();

//@ Cluster: no operations can be done on an offline cluster
Cluster.name;
Cluster.addInstance();
Cluster.checkInstanceState();
Cluster.describe();
Cluster.dissolve();
Cluster.forceQuorumUsingPartitionOf();
Cluster.getName();
Cluster.rejoinInstance();
Cluster.removeInstance();
Cluster.rescan();
Cluster.status();
Cluster.listRouters();
Cluster.removeRouterInstance();

//@ Cluster: disconnect() is ok on an offline cluster
Cluster.disconnect();

// Close session
session.close();

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
