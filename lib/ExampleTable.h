#ifndef ExampleTable_STRUCT_H
#define ExampleTable_STRUCT_H

#include <postgresql/libpq-fe.h>

struct ExampleTable {
  char* id;
  short small_int_col;
  int int_col;
  long long big_int_col;
  double decimal_col;
  double numeric_col;
  void* real_col;
  double double_col;
  void* serial_col;
  char* char_col;
  char* varchar_col;
  char* text_col;
  char* date_col;
  char* time_col;
  char* timestamp_col;
  char* timestamptz_col;
  int boolean_col;
  char* another_uuid;
  void* json_col;
  void* jsonb_col;
  int int_array_col;
  char* text_array_col;
  void* status_col;
  void* file_col;
};

void InsertExampleTable(PGconn* conn, struct ExampleTable u);
void UpdateExampleTable(PGconn* conn, struct ExampleTable u, const char* where_clause);
void DeleteExampleTable(PGconn* conn, const char* where_clause);
#endif // ExampleTable_STRUCT_H
