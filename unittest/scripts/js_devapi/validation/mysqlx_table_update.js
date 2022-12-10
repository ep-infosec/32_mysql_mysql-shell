//@ TableUpdate: valid operations after update
|All expected functions are available|
|No additional functions are available|

//@ TableUpdate: valid operations after set
|All expected functions are available|
|No additional functions are available|

//@ TableUpdate: valid operations after where
|All expected functions are available|
|No additional functions are available|

//@ TableUpdate: valid operations after orderBy
|All expected functions are available|
|No additional functions are available|

//@ TableUpdate: valid operations after limit
|All expected functions are available|
|No additional functions are available|

//@ TableUpdate: valid operations after bind
|All expected functions are available|
|No additional functions are available|

//@ TableUpdate: valid operations after execute
|All expected functions are available|
|No additional functions are available|

//@ Reusing CRUD with binding
|Updated Angel: 1|
|Updated Carol: 1|

//@# TableUpdate: Error conditions on update
||TableUpdate.update: Invalid number of arguments, expected 0 but got 1

//@# TableUpdate: Error conditions on set
||TableUpdate.set: Invalid number of arguments, expected 2 but got 0
||TableUpdate.set: Argument #1 is expected to be a string
||TableUpdate.set: Unsupported value received for table update operation on field "name", received: <Session:


//@# TableUpdate: Error conditions on where
||TableUpdate.where: Invalid number of arguments, expected 1 but got 0
||TableUpdate.where: Argument #1 is expected to be a string
||TableUpdate.where: Unterminated quoted string starting at position 8


//@# TableUpdate: Error conditions on orderBy
||TableUpdate.orderBy: Invalid number of arguments, expected at least 1 but got 0
||TableUpdate.orderBy: Argument #1 is expected to be a string or an array of strings
||TableUpdate.orderBy: Order criteria can not be empty
||TableUpdate.orderBy: Element #2 is expected to be a string
||TableUpdate.orderBy: Argument #2 is expected to be a string

//@# TableUpdate: Error conditions on limit
||TableUpdate.limit: Invalid number of arguments, expected 1 but got 0
||TableUpdate.limit: Argument #1 is expected to be an unsigned int

//@# TableUpdate: Error conditions on bind
||TableUpdate.bind: Invalid number of arguments, expected 2 but got 0
||TableUpdate.bind: Argument #1 is expected to be a string
||TableUpdate.bind: Unable to bind value for unexisting placeholder: another

//@# TableUpdate: Error conditions on execute
||TableUpdate.execute: Missing value bindings for the following placeholders: data, years
||TableUpdate.execute: Missing value bindings for the following placeholders: data


//@# TableUpdate: simple test
|Affected Rows: 1|
|Updated Record: aline 13|

//@ TableUpdate: test using expression
|Affected Rows: 1|
|Updated Record: aline 23|

//@ TableUpdate: test using limits
|Affected Rows: 2|
|With 16 Years: 3|
|With 15 Years: 1|

//@ TableUpdate: test full update with view object
|Updated Females: 4|
|All Females: 7|
