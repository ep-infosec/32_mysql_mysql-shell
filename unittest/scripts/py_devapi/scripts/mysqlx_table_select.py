# Assumptions: validate_crud_functions available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from __future__ import print_function
from mysqlsh import mysqlx

mySession = mysqlx.get_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')

schema = mySession.create_schema('js_shell_test')
mySession.set_current_schema('js_shell_test')

# Creates a test table with initial data
result = mySession.sql('create table table1 (name varchar(50), age integer, gender varchar(20), primary key (name, age, gender))').execute()
result = mySession.sql('create view view1 (my_name, my_age, my_gender) as select name, age, gender from table1;').execute()
table = schema.get_table('table1')

result = table.insert({"name": 'jack', "age": 17, "gender": 'male'}).execute()
result = table.insert({"name": 'adam', "age": 15, "gender": 'male'}).execute()
result = table.insert({"name": 'brian', "age": 14, "gender": 'male'}).execute()
result = table.insert({"name": 'alma', "age": 13, "gender": 'female'}).execute()
result = table.insert({"name": 'carol', "age": 14, "gender": 'female'}).execute()
result = table.insert({"name": 'donna', "age": 16, "gender": 'female'}).execute()
result = table.insert({"name": 'angel', "age": 14, "gender": 'male'}).execute()

# ----------------------------------------------
# Table.Select Unit Testing: Dynamic Behavior
# ----------------------------------------------
#@ TableSelect: valid operations after select
crud = table.select()
validate_crud_functions(crud, ['where', 'group_by', 'order_by', 'limit', 'lock_shared', 'lock_exclusive', 'bind', 'execute'])

#@ TableSelect: valid operations after where
crud = crud.where('age > 13')
validate_crud_functions(crud, ['group_by', 'order_by', 'limit', 'lock_shared', 'lock_exclusive', 'bind', 'execute'])

#@ TableSelect: valid operations after group_by
crud = crud.group_by(['name'])
validate_crud_functions(crud, ['having', 'order_by', 'limit', 'lock_shared', 'lock_exclusive', 'bind', 'execute'])

#@ TableSelect: valid operations after having
crud = crud.having('age > 10')
validate_crud_functions(crud, ['order_by', 'limit', 'lock_shared', 'lock_exclusive', 'bind', 'execute'])

#@ TableSelect: valid operations after order_by
crud = crud.order_by(['age'])
validate_crud_functions(crud, ['limit', 'lock_shared', 'lock_exclusive', 'bind', 'execute'])

#@ TableSelect: valid operations after limit
crud = crud.limit(1)
validate_crud_functions(crud, ['offset', 'lock_shared', 'lock_exclusive', 'bind', 'execute'])

#@ TableSelect: valid operations after offset
crud = crud.offset(1)
validate_crud_functions(crud, ['lock_shared', 'lock_exclusive', 'bind', 'execute'])

#@ TableSelect: valid operations after lock_shared
crud = table.select().where('name = :data').lock_shared()
validate_crud_functions(crud, ['bind', 'execute'])

#@ TableSelect: valid operations after lock_exclusive
crud = table.select().where('name = :data').lock_exclusive()
validate_crud_functions(crud, ['bind', 'execute'])

#@ TableSelect: valid operations after bind
crud = table.select().where('name = :data').bind('data', 'adam')
validate_crud_functions(crud, ['bind', 'execute'])

#@ TableSelect: valid operations after execute
result = crud.execute()
validate_crud_functions(crud, ['limit', 'bind', 'execute'])

#@ Reusing CRUD with binding
print(result.fetch_one().name + '\n')
result=crud.bind('data', 'alma').execute()
print(result.fetch_one().name + '\n')


# ----------------------------------------------
# Table.Select Unit Testing: Error Conditions
# ----------------------------------------------

#@# TableSelect: Error conditions on select
crud = table.select(5)
crud = table.select([])
crud = table.select(['name as alias', 5])
crud = table.select('name as alias', 5)

#@# TableSelect: Error conditions on where
crud = table.select().where()
crud = table.select().where(5)
crud = table.select().where('name = "whatever')

#@# TableSelect: Error conditions on group_by
crud = table.select().group_by()
crud = table.select().group_by(5)
crud = table.select().group_by([])
crud = table.select().group_by(['name', 5])
crud = table.select().group_by('name', 5)

#@# TableSelect: Error conditions on having
crud = table.select().group_by(['name']).having()
crud = table.select().group_by(['name']).having(5)

#@# TableSelect: Error conditions on order_by
crud = table.select().order_by()
crud = table.select().order_by(5)
crud = table.select().order_by([])
crud = table.select().order_by(['name', 5])
crud = table.select().order_by('name', 5)

#@# TableSelect: Error conditions on limit
crud = table.select().limit()
crud = table.select().limit('')

#@# TableSelect: Error conditions on offset
crud = table.select().limit(1).offset()
crud = table.select().limit(1).offset('')

#@# TableSelect: Error conditions on lock_shared
crud = table.select().lock_shared(5,1)
crud = table.select().lock_shared(5)

#@# TableSelect: Error conditions on lock_exclusive
crud = table.select().lock_exclusive(5,1)
crud = table.select().lock_exclusive(5)

#@# TableSelect: Error conditions on bind
crud = table.select().where('name = :data and age > :years').bind()
crud = table.select().where('name = :data and age > :years').bind(5, 5)
crud = table.select().where('name = :data and age > :years').bind('another', 5)

#@# TableSelect: Error conditions on execute
crud = table.select().where('name = :data and age > :years').execute()
crud = table.select().where('name = :data and age > :years').bind('years', 5).execute()


# ---------------------------------------
# Table.Select Unit Testing: Execution
# ---------------------------------------
records

#@ Table.Select All
#! [Table.Select All]
records = table.select().execute().fetch_all()
print("All:", len(records), "\n")
#! [Table.Select All]

#@ Table.Select Filtering
#! [Table.Select Filtering]
records = table.select().where('gender = "male"').execute().fetch_all()
print("Males:", len(records), "\n")

records = table.select().where('gender = "female"').execute().fetch_all()
print("Females:", len(records), "\n")

records = table.select().where('age = 13').execute().fetch_all()
print("13 Years:", len(records), "\n")

records = table.select().where('age = 14').execute().fetch_all()
print("14 Years:", len(records), "\n")

records = table.select().where('age < 17').execute().fetch_all()
print("Under 17:", len(records), "\n")

records = table.select().where('name like "a%"').execute().fetch_all()
print("Names With A:", len(records), "\n")

records = table.select().where('name LIKE "a%"').execute().fetch_all()
print("Names With A:", len(records), "\n")

records = table.select().where('NOT (age = 14)').execute().fetch_all()
print("Not 14 Years:", len(records), "\n")
#! [Table.Select Filtering]

#@ Table.Select Field Selection
#! [Table.Select Field Selection]
result = table.select(['name','age']).execute()
record = result.fetch_one()
columns = dir(record)
print(columns)
# In python, members are returned in alphabetic order
# We print the requested columns here (get_length and get_field are members too)
print('1-Metadata Length:', len(columns), '\n')
print('1-Metadata Field:', columns[5], '\n')
print('1-Metadata Field:', columns[0], '\n')

result = table.select(['age']).execute()
record = result.fetch_one()
columns = dir(record)
# In python, members are returned in alphabetic order
# We print the requested columns here (get_length and get_field are members too)
print('2-Metadata Length:', len(columns), '\n')
print('2-Metadata Field:', columns[0], '\n')
#! [Table.Select Field Selection]

#@ Table.Select Sorting
#! [Table.Select Sorting]
records = table.select().order_by(['name']).execute().fetch_all()
for index in range(7):
    print('Select Asc', index, ':', records[index].name, '\n')

records = table.select().order_by(['name desc']).execute().fetch_all()
for index in range(7):
    print('Select Desc', index, ':', records[index].name, '\n')
#! [Table.Select Sorting]

#@ Table.Select Limit and Offset
#! [Table.Select Limit and Offset]
records = table.select().limit(4).execute().fetch_all()
print('Limit-Offset 0 :', len(records), '\n')

for index in range(7):
    records = table.select().limit(4).offset(index + 1).execute().fetch_all()
    print('Limit-Offset', index + 1, ':', len(records), '\n')
#! [Table.Select Limit and Offset]

#@ Table.Select Parameter Binding through a View
view = schema.get_table('view1');
#! [Table.Select Parameter Binding]
records = view.select().where('my_age = :years and my_gender = :heorshe').bind('years', 13).bind('heorshe', 'female').execute().fetch_all()
print('Select Binding Length:', len(records), '\n')
print('Select Binding Name:', records[0].my_name, '\n')
#! [Table.Select Parameter Binding]

#@ Table.Select Basic vertical output
shell.options.resultFormat = "vertical"
table.select('name')

#@ Table.Select Check column align in vertical mode
shell.options.resultFormat = "vertical"
table.select('*').where('age > 15')

#@ Table.Select Check row with newline in vertical mode
shell.options.resultFormat = "vertical"
table.insert({ 'name': 'john\ndoe', 'age': 13, 'gender': 'male' }).execute();
table.select('*').where('age = 13')

#@ Table.Select Check switching between table and vertical mode 1
shell.options.resultFormat = "vertical"
table.select('name').where('age > 16')
#@<OUT> Table.Select Check switching between table and vertical mode 2
shell.options.resultFormat = "table"
table.select('name').where('age > 16')

#@ Table.Select Zerofill field as variable
mySession.sql('create table table2 (value int(5) zerofill)').execute()
table = schema.get_table('table2')
table.insert({"value": 1}).execute()
table.insert({"value": 12}).execute()
table.insert({"value": 12345}).execute()
table.insert({"value": 123456789}).execute()
records = table.select().execute().fetch_all()
for index in range(4):
  print('Variable value :', records[index].value, '\n')

#@ Table.Select Zerofill field display
mySession.sql('select * from table2')

#@<> WL12813 Collection.Find Collection
result = mySession.sql('create table wl12813 (`like` varchar(50), `notnested.like` varchar(50));').execute()
table = schema.get_table('wl12813')
result = table.insert().values('foo', 'bar').execute()
result = table.insert().values('foo', 'non nested bar').execute()
result = table.insert().values('bar', 'foo').execute()
result = table.insert().values('bar', 'non nested foo').execute()

#@ WL12813 Table Test 01
# Table.Select like as column
table.select().where('`like` = "foo"').execute()

#@ WL12813 Table Test 02 [USE:WL12813 Table Test 01]
# Table.Select unescaped like as column
table.select().where('like = "foo"').execute()

#@ WL12813 Table Test 03
# Table.Select escaped column
table.select().where('`notnested.like` = "foo"').execute()

#@ WL12813 Table Test 04
# Table.Select like as column and operator
table.select().where('`like` LIKE "%bar"').execute()

#@ WL12813 Table Test 05 [USE:WL12813 Table Test 04]
# Table.Select unescaped like as column and operator
table.select().where('like LIKE "%bar"').execute()

#@ WL12813 Table Test 06 [USE:WL12813 Table Test 01]
# Table.Select escaped column and like operator
table.select().where('`notnested.like` LIKE "%bar"').execute()

#@ WL12813 Table Test 07 [USE:WL12813 Table Test 04]
# Table.Select like as column, operator and placeholder
table.select().where('`like` LIKE :like').bind('like', '%bar').execute()

#@ WL12813 Table Test 08 [USE:WL12813 Table Test 04]
# Table.Select unescaped like as column, operator and placeholder
table.select().where('like LIKE :like').bind('like', '%bar').execute()

#@ WL12813 Table Test 09 [USE:WL12813 Table Test 01]
# Table.Select escaped column, with like as operator and placeholder
table.select().where('`notnested.like` LIKE :like').bind('like', '%bar').execute()

#@<> WL12767 Table.select {VER(>=8.0.17)}
result = mySession.sql('create table wl12767 (`overlaps` JSON, `list` JSON,`Name` varchar(50));').execute()
table = schema.get_table('wl12767')
result = table.insert().values('{"one":1, "two":2, "three":3}', '{"one":1,"two":2, "three":3}', "one").execute()
result = table.insert().values('{"one":1, "two":2, "three":3}', '{"four":4,"five":5, "six":6}', "two").execute()
result = table.insert().values('{"one":1, "three":3, "five":5}', '{"two":2,"four":4, "six":6}', "three").execute()
result = table.insert().values('{"one":1, "three":3, "five":5}', '{"three":3,"six":9, "nine":9}', "four").execute()
result = table.insert().values('{"one":1, "three":3, "five":5}', '{"three":6,"six":12, "nine":18}', "five").execute()
result = table.insert().values('{"one":[1,2,3]}', '{"one":[3,4,5]}',"six").execute()
result = table.insert().values('{"one":[1,2,3]}', '{"one":[1,2,3]}',"seven").execute()

#@ WL12767-TS1_1-01 {VER(>=8.0.17)}
# WL12767-TS6_1
table.select(["name"]).where("`overlaps` overlaps `list`").execute()

#@ WL12767-TS1_1-02 [USE:WL12767-TS1_1-01] {VER(>=8.0.17)}
# WL12767-TS3_1
table.select(["name"]).where("overlaps overlaps list").execute()

#@ WL12767-TS1_1-03 [USE:WL12767-TS1_1-01] {VER(>=8.0.17)}
table.select(["name"]).where("`list` overlaps `overlaps`").execute()

#@ WL12767-TS1_1-04 [USE:WL12767-TS1_1-01] {VER(>=8.0.17)}
# WL12767-TS3_1
table.select(["name"]).where("list overlaps overlaps").execute()

#@ WL12767-TS1_1-05 {VER(>=8.0.17)}
table.select(["name"]).where("`overlaps` not overlaps `list`").execute()

#@ WL12767-TS1_1-06 [USE:WL12767-TS1_1-05] {VER(>=8.0.17)}
# WL12767-TS2_1
# WL12767-TS3_1
table.select(["name"]).where("overlaps not OVERLAPS list").execute()

#@ WL12767-TS1_1-07 [USE:WL12767-TS1_1-05] {VER(>=8.0.17)}
table.select(["name"]).where("`list` not overlaps `overlaps`").execute()

#@ WL12767-TS1_1-08 [USE:WL12767-TS1_1-05] {VER(>=8.0.17)}
# WL12767-TS2_1
# WL12767-TS3_1
table.select(["name"]).where("list not OvErLaPs overlaps").execute()

#@ WL12767-TS5_1 {VER(>=8.0.17)}
table.select(["name"]).where("name OvErLaPs overlaps").execute()

#@<> BUG29794340 X DEVAPI: SUPPORT FOR JSON UNQUOTING EXTRACTION OPERATOR (->>)
shell.options['resultFormat'] = "tabbed"
mySession.sql("drop table if exists bug29794340").execute()
mySession.sql("create table bug29794340 (id integer primary key, doc json)").execute()
n = schema.get_table("bug29794340")
n.insert("id", "doc").values(1, '{ "name": "Bob" }').values(2, '{ "name": "Jake" }').values(3, '{ "name": "Mark" }').execute()

#@<OUT> BUG29794340: Right arrow operator
n.select("doc->'$.name' as names")

#@<OUT> BUG29794340: Two head right arrow operator
n.select("doc->>'$.name' as names")

#@ BUG29794340: Expected token type QUOTE
n.select("doc->>>'$.name' as names")

# Cleanup
mySession.drop_schema('js_shell_test')
mySession.close()
