-- MySQLShell dump 1.0.0  Distrib Ver 8.0.22 for osx10.15 on x86_64 - for MySQL 8.0.22 (Source distribution), for osx10.15 (x86_64)
--
-- Host: localhost
-- ------------------------------------------------------
-- Server version	5.7.30
--
-- Dumping user accounts
--

-- begin user 'test_all'@'localhost'
CREATE USER IF NOT EXISTS 'test_all'@'localhost' IDENTIFIED WITH 'mysql_native_password' REQUIRE NONE PASSWORD EXPIRE DEFAULT ACCOUNT UNLOCK;
-- end user 'test_all'@'localhost'

-- begin user 'test_mess'@'localhost'
CREATE USER IF NOT EXISTS 'test_mess'@'localhost' IDENTIFIED WITH 'mysql_native_password' REQUIRE NONE PASSWORD EXPIRE DEFAULT ACCOUNT UNLOCK;
-- end user 'test_mess'@'localhost'

-- begin user 'test_mysqlx_all'@'localhost'
CREATE USER IF NOT EXISTS 'test_mysqlx_all'@'localhost' IDENTIFIED WITH 'mysql_native_password' REQUIRE NONE PASSWORD EXPIRE DEFAULT ACCOUNT UNLOCK;
-- end user 'test_mysqlx_all'@'localhost'

-- begin user 'test_mysql_all'@'localhost'
CREATE USER IF NOT EXISTS 'test_mysql_all'@'localhost' IDENTIFIED WITH 'mysql_native_password' REQUIRE NONE PASSWORD EXPIRE DEFAULT ACCOUNT UNLOCK;
-- end user 'test_mysql_all'@'localhost'

-- begin user 'test_mysql_all_user'@'localhost'
CREATE USER IF NOT EXISTS 'test_mysql_all_user'@'localhost' IDENTIFIED WITH 'mysql_native_password' REQUIRE NONE PASSWORD EXPIRE DEFAULT ACCOUNT UNLOCK;
-- end user 'test_mysql_all_user'@'localhost'

-- begin user 'test_mysql_mixed'@'localhost'
CREATE USER IF NOT EXISTS 'test_mysql_mixed'@'localhost' IDENTIFIED WITH 'mysql_native_password' REQUIRE NONE PASSWORD EXPIRE DEFAULT ACCOUNT UNLOCK;
-- end user 'test_mysql_mixed'@'localhost'

-- begin user 'test_mysql_none'@'localhost'
CREATE USER IF NOT EXISTS 'test_mysql_none'@'localhost' IDENTIFIED WITH 'mysql_native_password' REQUIRE NONE PASSWORD EXPIRE DEFAULT ACCOUNT UNLOCK;
-- end user 'test_mysql_none'@'localhost'

-- begin user 'test_mysql_partial'@'localhost'
CREATE USER IF NOT EXISTS 'test_mysql_partial'@'localhost' IDENTIFIED WITH 'mysql_native_password' REQUIRE NONE PASSWORD EXPIRE DEFAULT ACCOUNT UNLOCK;
-- end user 'test_mysql_partial'@'localhost'

-- begin user 'test_noprivs'@'localhost'
CREATE USER IF NOT EXISTS 'test_noprivs'@'localhost' IDENTIFIED WITH 'mysql_native_password' REQUIRE NONE PASSWORD EXPIRE DEFAULT ACCOUNT UNLOCK;
-- end user 'test_noprivs'@'localhost'

-- begin user 'test_sys_all'@'localhost'
CREATE USER IF NOT EXISTS 'test_sys_all'@'localhost' IDENTIFIED WITH 'mysql_native_password' REQUIRE NONE PASSWORD EXPIRE DEFAULT ACCOUNT UNLOCK;
-- end user 'test_sys_all'@'localhost'

-- begin grants 'test_all'@'localhost'
GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, SHUTDOWN, PROCESS, REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, CREATE TABLESPACE ON *.* TO 'test_all'@'localhost';
-- end grants 'test_all'@'localhost'

-- begin grants 'test_mess'@'localhost'
GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, SHUTDOWN, PROCESS, REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, CREATE TABLESPACE ON *.* TO 'test_mess'@'localhost';
GRANT SELECT, DELETE ON `mysql`.* TO 'test_mess'@'localhost';
GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE VIEW, SHOW VIEW ON `mysql`.`user` TO 'test_mess'@'localhost';
-- end grants 'test_mess'@'localhost'

-- begin grants 'test_mysqlx_all'@'localhost'
GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, SHUTDOWN, PROCESS, REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, CREATE TABLESPACE ON *.* TO 'test_mysqlx_all'@'localhost';
-- end grants 'test_mysqlx_all'@'localhost'

-- begin grants 'test_mysql_all'@'localhost'
GRANT USAGE ON *.* TO 'test_mysql_all'@'localhost';
-- end grants 'test_mysql_all'@'localhost'

-- begin grants 'test_mysql_all_user'@'localhost'
GRANT USAGE ON *.* TO 'test_mysql_all_user'@'localhost';
GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE VIEW, SHOW VIEW ON `mysql`.`user` TO 'test_mysql_all_user'@'localhost' WITH GRANT OPTION;
-- end grants 'test_mysql_all_user'@'localhost'

-- begin grants 'test_mysql_mixed'@'localhost'
GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, SHUTDOWN, PROCESS, REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, CREATE TABLESPACE ON *.* TO 'test_mysql_mixed'@'localhost';
-- end grants 'test_mysql_mixed'@'localhost'

-- begin grants 'test_mysql_none'@'localhost'
GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, SHUTDOWN, PROCESS, REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, CREATE TABLESPACE ON *.* TO 'test_mysql_none'@'localhost';
-- end grants 'test_mysql_none'@'localhost'

-- begin grants 'test_mysql_partial'@'localhost'
GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, SHUTDOWN, PROCESS, REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, CREATE TABLESPACE ON *.* TO 'test_mysql_partial'@'localhost';
-- end grants 'test_mysql_partial'@'localhost'

-- begin grants 'test_noprivs'@'localhost'
GRANT USAGE ON *.* TO 'test_noprivs'@'localhost';
-- end grants 'test_noprivs'@'localhost'

-- begin grants 'test_sys_all'@'localhost'
GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, SHUTDOWN, PROCESS, REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, CREATE TABLESPACE ON *.* TO 'test_sys_all'@'localhost';
-- end grants 'test_sys_all'@'localhost'

-- End of dumping user accounts
