# Connecting to MySQL Server and working with a Collection
from mysqlsh import mysqlx

# Connect to server
mySession = mysqlx.get_session( {
'host': 'localhost', 'port': 33060,
'user': 'mike', 'password': 'paSSw0rd'} )

myDb = mySession.get_schema('test')

# Create a new collection 'my_collection'
myColl = myDb.create_collection('my_collection')

# Insert documents
myColl.add({'_id': '1', 'name': 'Sakila', 'age': 15}).execute()
myColl.add({'_id': '2', 'name': 'Susanne', 'age': 24}).execute()
myColl.add({'_id': '3', 'name': 'Mike', 'age': 39}).execute()

# Find a document
docs = myColl.find('name like :param1 AND age < :param2') \
          .limit(1) \
          .bind('param1','S%') \
          .bind('param2',20) \
          .execute()

# Print document
doc = docs.fetch_one()
print(doc)

# Drop the collection
myDb.drop_collection('my_collection')
