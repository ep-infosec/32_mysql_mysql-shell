#@<OUT> Help on LockContention
NAME
      LockContention - Row locking mode constants.

SYNTAX
      mysqlx.LockContention

DESCRIPTION
      These constants are used to indicate the locking mode to be used at the
      lock_shared and lock_exclusive functions of the TableSelect and
      CollectionFind objects.

PROPERTIES
      DEFAULT
            A default locking mode.

      NOWAIT
            A locking read never waits to acquire a row lock. The query
            executes immediately, failing with an error if a requested row is
            locked.

      SKIP_LOCKED
            A locking read never waits to acquire a row lock. The query
            executes immediately, removing locked rows from the result set.

FUNCTIONS
      help([member])
            Provides help about this class and it's members

#@<OUT> Help on LockContention.DEFAULT
NAME
      DEFAULT - A default locking mode.

SYNTAX
      mysqlx.LockContention.DEFAULT

#@<OUT> Help on LockContention.NOWAIT
NAME
      NOWAIT - A locking read never waits to acquire a row lock. The query
               executes immediately, failing with an error if a requested row
               is locked.

SYNTAX
      mysqlx.LockContention.NOWAIT

#@<OUT> Help on LockContention.SKIP_LOCKED
NAME
      SKIP_LOCKED - A locking read never waits to acquire a row lock. The query
                    executes immediately, removing locked rows from the result
                    set.

SYNTAX
      mysqlx.LockContention.SKIP_LOCKED

#@<OUT> Help on LockContention.help
NAME
      help - Provides help about this class and it's members

SYNTAX
      mysqlx.LockContention.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

