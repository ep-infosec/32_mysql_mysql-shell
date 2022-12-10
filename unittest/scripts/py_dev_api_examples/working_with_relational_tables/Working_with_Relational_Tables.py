# Working with Relational Tables
from mysqlsh import mysqlx

# Connect to server using a connection URL
mySession = mysqlx.get_session( {
  'host': 'localhost', 'port': 33060,
  'user': 'mike', 'password': 'paSSw0rd'} )

myDb = mySession.get_schema('test')

# Accessing an existing table
myTable = myDb.get_table('my_table')

# Insert SQL Table data
myTable.insert(['name','birthday','age']) \
  .values('Sakila', mysqlx.date_value(2000, 5, 27), 16).execute()

# Find a row in the SQL Table
myResult = myTable.select(['_id', 'name', 'birthday']) \
  .where('name like :name AND age < :age') \
  .bind('name', 'S%') \
  .bind('age', 20).execute()

# Print result
print(myResult.fetch_all())
