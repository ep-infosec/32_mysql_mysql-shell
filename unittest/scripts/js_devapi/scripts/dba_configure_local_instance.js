// Assumptions: smart deployment rountines available

//@<OUT> create GR admin account using configureLocalInstance
var cnfPath1 = __sandbox_dir + __mysql_sandbox_port1 + "/my.cnf";
dba.configureLocalInstance("root@localhost:"+__mysql_sandbox_port1, {mycnfPath: cnfPath1, password:'root', clusterAdmin: "gr_user", clusterAdminPassword: "root"});

//@ Error: user has no privileges to run the configure command (BUG#26609909)
shell.connect(__sandbox_uri1);
// Remove select privilege to make sure an error is thrown
session.runSql("SET sql_log_bin = 0");
session.runSql("REVOKE SELECT on *.* FROM 'gr_user'@'%'");
session.runSql("SET sql_log_bin = 1");
dba.configureLocalInstance("gr_user@localhost:"+__mysql_sandbox_port1, {mycnfPath: cnfPath1, password:'root'});
//@ Error: session user has privileges to run the configure command but we pass it an existing clusterAdmin user that doesn't have enough privileges (BUG#26979375)
dba.configureLocalInstance("root@localhost:"+__mysql_sandbox_port1, {mycnfPath: cnfPath1, password:'root', clusterAdmin: 'gr_user', clusterAdminPassword: "root"});

//@ Session user has privileges to run the configure command and we pass it an non existing clusterAdmin user(BUG#26979375)
dba.configureLocalInstance("root@localhost:"+__mysql_sandbox_port1, {mycnfPath: cnfPath1, password:'root', clusterAdmin: 'gr_user2', clusterAdminPassword: "root"});

// restore select privilege to gr_user
session.runSql("SET sql_log_bin = 0");
session.runSql("GRANT SELECT on *.* TO 'gr_user'@'%'");
session.runSql("SET sql_log_bin = 1");
session.close();

//@ create cluster using cluster admin account (BUG#26523629)
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'gr_user', password: 'root'});
var cluster = dba.createCluster('devCluster', {memberSslMode:'REQUIRED', clearReadOnly: true, gtidSetIsComplete: true});

//@ Validates the createCluster successfully configured the grLocal member of the instance addresses
var gr_local_port = __mysql_sandbox_port1 * 10 + 1;
var res = session.runSql('select json_unquote(addresses->\'$.grLocal\') from mysql_innodb_cluster_metadata.instances where addresses->\'$.mysqlClassic\' = \'localhost:' + __mysql_sandbox_port1 + '\'');
var row = res.fetchOne();
print (row[0]);

//@ Adding the remaining instances
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
cluster.addInstance(__sandbox_uri2, {'label': 'second_sandbox'});
cluster.addInstance(__sandbox_uri3, {'label': 'third_sandbox'});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> Healthy cluster status
cluster.status();

//@ Kill instance, will not auto join after start
dba.killSandboxInstance(__mysql_sandbox_port3, {sandboxDir:__sandbox_dir});
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE");

//@ Start instance, will not auto join the cluster
dba.startSandboxInstance(__mysql_sandbox_port3, {sandboxDir: __sandbox_dir});
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

//@<OUT> Still missing 3rd instance
os.sleep(5);
cluster.status();

//@#: Rejoins the instance
cluster.rejoinInstance({dbUser: "root", host: "localhost", port:__mysql_sandbox_port3}, {memberSslMode: "AUTO", "password": "root"});
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<OUT> Instance is back
cluster.status();

//@ Persist the GR configuration
var cnfPath3 = __sandbox_dir + __mysql_sandbox_port3 + "/my.cnf";
var result = dba.configureLocalInstance('root@localhost:' + __mysql_sandbox_port3, {mycnfPath: cnfPath3, DBPASSWORD:'root'});
print (result.status)

//@ Kill instance, will auto join after start
dba.killSandboxInstance(__mysql_sandbox_port3, {sandboxDir:__sandbox_dir});
testutil.waitMemberState(__mysql_sandbox_port3, "UNREACHABLE");

//@ Start instance, will auto join the cluster
dba.startSandboxInstance(__mysql_sandbox_port3, {sandboxDir: __sandbox_dir});
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

session.close();

//@<OUT> Check saved auto_inc settings are restored
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port3, user: 'root', password: 'root'});
session.runSql("show global variables like 'auto_increment_%'").fetchAll();
session.close();

