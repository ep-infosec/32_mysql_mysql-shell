// Assumptions: validate_crud_functions available

var mysqlx = require('mysqlx');

var mySession = mysqlx.getSession(__uripwd);

ensure_schema_does_not_exist(mySession, 'js_shell_test');

var schema = mySession.createSchema('js_shell_test');
mySession.setCurrentSchema('js_shell_test');

// Creates a test table with initial data
var result = mySession.sql('create table table1 (name varchar(50) primary key, age integer, gender varchar(20));').execute();
var result = mySession.sql('create view view1 (my_name, my_age, my_gender) as select name, age, gender from table1;').execute();
table = schema.getTable('table1');

var result = table.insert({ name: 'jack', age: 17, gender: 'male' }).execute();
var result = table.insert({ name: 'adam', age: 15, gender: 'male' }).execute();
var result = table.insert({ name: 'brian', age: 14, gender: 'male' }).execute();
var result = table.insert({ name: 'alma', age: 13, gender: 'female' }).execute();
var result = table.insert({ name: 'carol', age: 14, gender: 'female' }).execute();
var result = table.insert({ name: 'donna', age: 16, gender: 'female' }).execute();
var result = table.insert({ name: 'angel', age: 14, gender: 'male' }).execute();

// ------------------------------------------------
// table.delete Unit Testing: Dynamic Behavior
// ------------------------------------------------
//@ TableDelete: valid operations after delete
var crud = table.delete();
validate_crud_functions(crud, ['where', 'orderBy', 'limit', 'execute']);

//@ TableDelete: valid operations after where
var crud = crud.where("id < 100");
validate_crud_functions(crud, ['orderBy', 'limit', 'bind', 'execute']);

//@ TableDelete: valid operations after orderBy
var crud = crud.orderBy(['name']);
validate_crud_functions(crud, ['limit', 'bind', 'execute']);

//@ TableDelete: valid operations after limit
var crud = crud.limit(1);
validate_crud_functions(crud, ['bind', 'execute']);

//@ TableDelete: valid operations after bind
var crud = table.delete().where('name = :data').bind('data', 'donna');
validate_crud_functions(crud, ['bind', 'execute']);

//@ TableDelete: valid operations after execute
var result = crud.execute();
validate_crud_functions(crud, ['limit', 'bind', 'execute']);

//@ Reusing CRUD with binding
print('Deleted donna:', result.affectedItemsCount, '\n');
var result = crud.bind('data', 'alma').execute();
print('Deleted alma:', result.affectedItemsCount, '\n');

// ----------------------------------------------
// table.delete Unit Testing: Error Conditions
// ----------------------------------------------

//@# TableDelete: Error conditions on delete
crud = table.delete(5);

//@# TableDelete: Error conditions on where
crud = table.delete().where();
crud = table.delete().where(5);
crud = table.delete().where('test = "2');

//@# TableDelete: Error conditions on orderBy
crud = table.delete().orderBy();
crud = table.delete().orderBy(5);
crud = table.delete().orderBy([]);
crud = table.delete().orderBy(['name', 5]);
crud = table.delete().orderBy('name', 5);

//@# TableDelete: Error conditions on limit
crud = table.delete().limit();
crud = table.delete().limit('');

//@# TableDelete: Error conditions on bind
crud = table.delete().where('name = :data and age > :years').bind();
crud = table.delete().where('name = :data and age > :years').bind(5, 5);
crud = table.delete().where('name = :data and age > :years').bind('another', 5);

//@# TableDelete: Error conditions on execute
crud = table.delete().where('name = :data and age > :years').execute();
crud = table.delete().where('name = :data and age > :years').bind('years', 5).execute()

// ---------------------------------------
// table.delete Unit Testing: Execution
// ---------------------------------------

//@ TableDelete: delete under condition
//! [TableDelete: delete under condition]
var result = table.delete().where('age = 15').execute();
print('Affected Rows:', result.affectedItemsCount, '\n');

var records = table.select().execute().fetchAll();
print('Records Left:', records.length, '\n');
//! [TableDelete: delete under condition]

//@ TableDelete: delete with binding
//! [TableDelete: delete with binding]
var result = table.delete().where('gender = :heorshe').limit(2).bind('heorshe', 'male').execute();
print('Affected Rows:', result.affectedItemsCount, '\n');

var records = table.select().execute().fetchAll();
print('Records Left:', records.length, '\n');
//! [TableDelete: delete with binding]

//@ TableDelete: full delete with a view object
var view = schema.getTable('view1');
//! [TableDelete: full delete]
var result = view.delete().execute();
print('Affected Rows:', result.affectedItemsCount, '\n');

// Deletion is of course reflected on the target table
var records = table.select().execute().fetchAll();
print('Records Left:', records.length, '\n');
//! [TableDelete: full delete]

//@ TableDelete: with limit 0
var result = table.insert({ name: 'adam', age: 15, gender: 'male' }).execute();
var result = table.insert({ name: 'brian', age: 14, gender: 'male' }).execute();
var result = table.insert({ name: 'alma', age: 13, gender: 'female' }).execute();
var result = table.insert({ name: 'carol', age: 14, gender: 'female' }).execute();
var result = table.insert({ name: 'donna', age: 16, gender: 'female' }).execute();

var records = table.select().execute().fetchAll();
print('Records Left:', records.length, '\n');

//@ TableDelete: with limit 1
//! [TableDelete: with limit]
var result = table.delete().limit(2).execute();
print('Affected Rows:', result.affectedItemsCount, '\n');

var records = table.select().execute().fetchAll();
print('Records Left:', records.length, '\n');
//! [TableDelete: with limit]

//@ TableDelete: with limit 2
var result = table.delete().limit(2).execute();
print('Affected Rows:', result.affectedItemsCount, '\n');

var records = table.select().execute().fetchAll();
print('Records Left:', records.length, '\n');

//@ TableDelete: with limit 3
var result = table.delete().limit(2).execute();
print('Affected Rows:', result.affectedItemsCount, '\n');

var records = table.select().execute().fetchAll();
print('Records Left:', records.length, '\n');

// Cleanup
mySession.dropSchema('js_shell_test');
mySession.close();
