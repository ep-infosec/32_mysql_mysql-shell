# Assumptions: validate_crud_functions available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from __future__ import print_function
from mysqlsh import mysqlx

mySession = mysqlx.get_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

schema = mySession.create_schema('js_shell_test')
mySession.set_current_schema('js_shell_test')

# Creates a test table
result = mySession.sql('create table table1 (name varchar(50), age integer, gender varchar(20), primary key (name, age, gender))').execute()
result = mySession.sql('create view view1 (my_name, my_age, my_gender) as select name, age, gender from table1;').execute()

table = schema.get_table('table1')

# ---------------------------------------------
# Table.insert Unit Testing: Dynamic Behavior
# ---------------------------------------------
#@ TableInsert: valid operations after empty insert
crud = table.insert()
validate_crud_functions(crud, ['values'])

#@ TableInsert: valid operations after empty insert and values
crud = crud.values('john', 25, 'male')
validate_crud_functions(crud, ['values', 'execute'])

#@ TableInsert: valid operations after empty insert and values 2
crud = crud.values('alma', 23, 'female')
validate_crud_functions(crud, ['values', 'execute'])

#@ TableInsert: valid operations after insert with field list
crud = table.insert(['name', 'age', 'gender'])
validate_crud_functions(crud, ['values'])

#@ TableInsert: valid operations after insert with field list and values
crud = crud.values('john', 25, 'male')
validate_crud_functions(crud, ['values', 'execute'])

#@ TableInsert: valid operations after insert with field list and values 2
crud = crud.values('alma', 23, 'female')
validate_crud_functions(crud, ['values', 'execute'])

#@ TableInsert: valid operations after insert with multiple fields
crud = table.insert('name', 'age', 'gender')
validate_crud_functions(crud, ['values'])

#@ TableInsert: valid operations after insert with multiple fields and values
crud = crud.values('john', 25, 'male')
validate_crud_functions(crud, ['values', 'execute'])

#@ TableInsert: valid operations after insert with multiple fields and values 2
crud = crud.values('alma', 23, 'female')
validate_crud_functions(crud, ['values', 'execute'])

#@ TableInsert: valid operations after insert with fields and values
crud = table.insert({"name":'john', "age":25, "gender":'male'})
validate_crud_functions(crud, ['execute'])

#@ TableInsert: valid operations after execute
result = crud.execute()
validate_crud_functions(crud, ['execute'])


# -------------------------------------------
# Table.insert Unit Testing: Error Conditions
# -------------------------------------------

#@# TableInsert: Error conditions on insert
crud = table.insert(45)
crud = table.insert('name', 28)
crud = table.insert(['name', 28])

crud = table.insert(['name', 'age', 'gender']).values([5]).execute()
crud = table.insert(['name', 'age', 'gender']).values('carol', mySession).execute()
crud = table.insert(['name', 'id', 'gender']).values('carol', 20, 'female').execute()


# ---------------------------------------
# Table.Find Unit Testing: Execution
# ---------------------------------------

#@ Table.insert execution
#! [TableInsert: insert()]
result = table.insert().values('jack', 17, 'male').execute()
print("Affected Rows No Columns:", result.affected_items_count, "\n")
#! [TableInsert: insert()]

#! [TableInsert: insert(list)]
result = table.insert(['age', 'name', 'gender']).values(21, 'john', 'male').execute()
print("Affected Rows Columns:", result.affected_items_count, "\n")
#! [TableInsert: insert(list)]

#! [TableInsert: insert(str...)]
insert = table.insert('name', 'age', 'gender')
crud = insert.values('clark', 22,'male')
crud = insert.values('mary', 13,'female')
result = insert.execute()
print("Affected Rows Multiple Values:", result.affected_items_count, "\n")
#! [TableInsert: insert(str...)]

#! [TableInsert: insert(JSON)]
result = table.insert({'age':14, 'name':'jackie', 'gender': 'female'}).execute()
print("Affected Rows Document:", result.affected_items_count, "\n")
#! [TableInsert: insert(JSON)]

#@ Table.insert execution on a View
view = schema.get_table('view1')
result = view.insert({ 'my_age': 15, 'my_name': 'jhonny', 'my_gender': 'male' }).execute()
print("Affected Rows Through View:", result.affected_items_count, "\n")


# Cleanup
mySession.drop_schema('js_shell_test')
mySession.close()
