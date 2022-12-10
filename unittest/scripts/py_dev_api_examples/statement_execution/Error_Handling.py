from mysqlsh import mysqlx

mySession

try:
        # Connect to server on localhost
        mySession = mysqlx.get_session( {
                'host': 'localhost', 'port': 33060,
                'user': 'mike', 'password': 'paSSw0rd' } )
except Exception as err:
        print('The database session could not be opened: %s' % str(err))

try:
        myDb = mySession.get_schema('test')
        
        # Use the collection 'my_collection'
        myColl = myDb.get_collection('my_collection')
        
        # Find a document
        myDoc = myColl.find('name like :param').limit(1).bind('param','S%').execute()
        
        # Print document
        print(myDoc.first())
except Exception as err:
        print('The following error occurred: %s' % str(err))
finally:
        # Close the session in any case
        mySession.close()
