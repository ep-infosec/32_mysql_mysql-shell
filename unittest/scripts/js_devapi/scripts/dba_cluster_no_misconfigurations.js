// Assumptions: smart deployment routines available
//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

//@ Dba.createCluster
var cluster = dba.createCluster('dev', {memberSslMode:'REQUIRED'});

// Dba.dissolve
cluster.dissolve({force:true});
cluster.disconnect();

// Regression for BUG#26248116 : MYSQLPROVISION DOES NOT USE SECURE CONNECTIONS BY DEFAULT
// Test can only be performed if SSL is supported.
//@ Create cluster requiring secure connections (if supported)
var result = session.runSql("SHOW GLOBAL VARIABLES LIKE 'require_secure_transport'");
var row = result.fetchOne();
var req_sec_trans = row[1];
session.runSql("SET @@global.require_secure_transport = ON");
var cluster = dba.createCluster('dev', {memberSslMode: 'REQUIRED', clearReadOnly: true, gtidSetIsComplete: true});

//@ Add instance requiring secure connections (if supported)
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
cluster.addInstance(__sandbox_uri2);

//@ Dissolve cluster requiring secure connections (if supported)
cluster.dissolve({force:true});
session.runSql("SET @@global.require_secure_transport = '" + req_sec_trans + "'");

session.close();
cluster.disconnect();

//@ Finalization
// Will delete the sandboxes ONLY if this test was executed standalone
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
