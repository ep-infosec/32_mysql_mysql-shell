function print_result(res) {
  if (res.hasData()) {
    // SELECT
    var columns = res.getColumns();
    var record = res.fetchOne();

    while (record){
      for (index in columns){
        print (columns[index].getColumnName() + ": " + record[index] + "\n");
      }

      // Get the next record
      record = res.fetchOne();
    }

  } else {
    // INSERT, UPDATE, DELETE, ...
    print('Rows affected: ' + res.getAffectedItemsCount());
  }
}

print_result(mySession.sql('DELETE FROM users WHERE age > 40').execute());
print_result(mySession.sql('SELECT * FROM users WHERE age = 40').execute());
