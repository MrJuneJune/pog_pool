#include "ExampleTable.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void InsertExampleTable(PGconn* conn, struct ExampleTable u) {
  char query[1024];
  snprintf(query, sizeof(query),
    "INSERT INTO ExampleTable (id, small_int_col, int_col, big_int_col, decimal_col, numeric_col, real_col, double_col, serial_col, char_col, varchar_col, text_col, date_col, time_col, timestamp_col, timestamptz_col, boolean_col, another_uuid, json_col, jsonb_col, int_array_col, text_array_col, status_col, file_col) "
    "VALUES ('%s', %d, %d, %d, %f, %f, '%s', %f, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s', '%s', '%s', %d, '%s', '%s', '%s');",
    u.id, u.small_int_col, u.int_col, u.big_int_col, u.decimal_col, u.numeric_col, u.real_col, u.double_col, u.serial_col, u.char_col, u.varchar_col, u.text_col, u.date_col, u.time_col, u.timestamp_col, u.timestamptz_col, u.boolean_col, u.another_uuid, u.json_col, u.jsonb_col, u.int_array_col, u.text_array_col, u.status_col, u.file_col);
  PGresult* res = PQexec(conn, query);
  PQclear(res);
}

void UpdateExampleTable(PGconn* conn, struct ExampleTable u, const char* where_clause) {
  char query[1024];
  snprintf(query, sizeof(query),
    "UPDATE ExampleTable "
    "SET id='%s', small_int_col=%d, int_col=%d, big_int_col=%d, decimal_col=%f, numeric_col=%f, real_col='%s', double_col=%f, serial_col='%s', char_col='%s', varchar_col='%s', text_col='%s', date_col='%s', time_col='%s', timestamp_col='%s', timestamptz_col='%s', boolean_col=%d, another_uuid='%s', json_col='%s', jsonb_col='%s', int_array_col=%d, text_array_col='%s', status_col='%s', file_col='%s' WHERE %s;",
    u.id, u.small_int_col, u.int_col, u.big_int_col, u.decimal_col, u.numeric_col, u.real_col, u.double_col, u.serial_col, u.char_col, u.varchar_col, u.text_col, u.date_col, u.time_col, u.timestamp_col, u.timestamptz_col, u.boolean_col, u.another_uuid, u.json_col, u.jsonb_col, u.int_array_col, u.text_array_col, u.status_col, u.file_col, where_clause);
  PGresult* res = PQexec(conn, query);
  PQclear(res);
}

void DeleteExampleTable(PGconn* conn, const char* where_clause) {
  char query[512];
  snprintf(query, sizeof(query),
    "DELETE FROM ExampleTable WHERE %s;",
    where_clause);
  PGresult* res = PQexec(conn, query);
  PQclear(res);
}

int main()
{
 return 0; 
}
