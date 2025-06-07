#include "pog_pool/connection.h"
#include "pog_pool/auto_generate.h"
#include "../build/models.h"

volatile ConnectionPool* connection_pool;

int main()
{
  // assumping you are running make from make folder
  const char *config_path = "./example/pog_pool_config.yml";
  PogPoolConfig pogPoolConfig = {0};

  ParseConfig(config_path, &pogPoolConfig);

  ConnectionPool connection_pool_real = {0};
  connection_pool = &connection_pool_real;

  InitPool(connection_pool, pogPoolConfig.database_url);
  PGconn *pg_conn = BorrowConnection(connection_pool);
  RunSQLFile(pg_conn, "models/ExampleTable.sql");

  ExampleTable example = {
    .id = "b6d4c431-f327-4a4a-9345-320aa3cd7e51",
    .small_int_col = 1,
    .int_col = 42,
    .big_int_col = 9000000000LL,
    .decimal_col = 12.34,
    .numeric_col = 56.789,
    .real_col = 1.23,
    .double_col = 9.87,
    .serial_col = NULL,
  
    .char_col = "char_data",
    .varchar_col = "varchar_data",
    .text_col = "This is a long text",
  
    .date_col = "2025-06-05",
    .time_col = "12:34:56",
    .timestamp_col = "2025-06-05 12:34:56",
    .timestamptz_col = "2025-06-05 12:34:56+00",
  
    .boolean_col = 1,
    .another_uuid = "d1b355c0-f348-4bcf-b3df-ef95b3a8a3ad",
  
    .json_col = "{\"key\": \"value\"}",
    .jsonb_col = "{\"key\": \"value\"}",
  
    .int_array_col = "{1,2,3}",
    .text_array_col = "{\"apple\",\"banana\"}",
  
    .status_col = "active",
    .file_col = "\\x68656c6c6f" 
  };
  
  InsertExampleTable(pg_conn, example);

  ExampleTableQuery et;
  ExampleTable* e;

  et = QueryExampleTable(pg_conn, "*", "1=1");
  if (et.ExampleTable != NULL)
  {
    e = et.ExampleTable;
  
    printf("id: %s\n", e->id);
    printf("small_int_col: %d\n", e->small_int_col);
    printf("int_col: %d\n", e->int_col);
    printf("big_int_col: %lld\n", e->big_int_col);
    printf("decimal_col: %.2f\n", e->decimal_col);
    printf("numeric_col: %.3f\n", e->numeric_col);
    printf("real_col: %i\n", e->real_col);  // stored as string
    printf("double_col: %f\n", e->double_col);
    // serial_col is auto-generated — skip or print if needed
    // printf("serial_col: %s\n", (char*)e->serial_col);
  
    printf("char_col: %s\n", e->char_col);
    printf("varchar_col: %s\n", e->varchar_col);
    printf("text_col: %s\n", e->text_col);
  
    printf("date_col: %s\n", e->date_col);
    printf("time_col: %s\n", e->time_col);
    printf("timestamp_col: %s\n", e->timestamp_col);
    printf("timestamptz_col: %s\n", e->timestamptz_col);
  
    printf("boolean_col: %s\n", e->boolean_col ? "TRUE" : "FALSE");
    printf("another_uuid: %s\n", e->another_uuid);
  
    printf("json_col: %s\n", (char*)e->json_col);
    printf("jsonb_col: %s\n", (char*)e->jsonb_col);
  
    printf("int_array_col: %s\n", e->int_array_col);
    printf("text_array_col: %s\n", e->text_array_col);
  
    printf("status_col: %s\n", (char*)e->status_col);
    printf("file_col (hex): %s\n", (char*)e->file_col);

    printf("\n\nSerialized: %s\n", SerializeExampleTable(*et.ExampleTable));
  }
  else
  {
    printf("Does not exist...\n");
  }

  // ID only
  printf("ID ONLY\n\n\n");
  et = QueryExampleTable(pg_conn, "id", "1=1");
  if (et.ExampleTable != NULL)
  {
    e = et.ExampleTable;
  
    printf("id: %s\n", e->id);
    printf("small_int_col: %d\n", e->small_int_col);
    printf("int_col: %d\n", e->int_col);
    printf("big_int_col: %lld\n", e->big_int_col);
    printf("decimal_col: %.2f\n", e->decimal_col);
    printf("numeric_col: %.3f\n", e->numeric_col);
    printf("real_col: %i\n", e->real_col);  // stored as string
    printf("double_col: %f\n", e->double_col);
    // serial_col is auto-generated — skip or print if needed
    // printf("serial_col: %s\n", (char*)e->serial_col);
  
    printf("char_col: %s\n", e->char_col);
    printf("varchar_col: %s\n", e->varchar_col);
    printf("text_col: %s\n", e->text_col);
  
    printf("date_col: %s\n", e->date_col);
    printf("time_col: %s\n", e->time_col);
    printf("timestamp_col: %s\n", e->timestamp_col);
    printf("timestamptz_col: %s\n", e->timestamptz_col);
  
    printf("boolean_col: %s\n", e->boolean_col ? "TRUE" : "FALSE");
    printf("another_uuid: %s\n", e->another_uuid);
  
    printf("json_col: %s\n", (char*)e->json_col);
    printf("jsonb_col: %s\n", (char*)e->jsonb_col);
  
    printf("int_array_col: %s\n", e->int_array_col);
    printf("text_array_col: %s\n", e->text_array_col);
  
    printf("status_col: %s\n", (char*)e->status_col);
    printf("file_col (hex): %s\n", (char*)e->file_col);

    printf("\n\nSerialized: %s\n", SerializeExampleTable(*et.ExampleTable));
  }

  char where_clause[512];
  sprintf(where_clause, "id = \'%s\'", example.id);
  DeleteExampleTable(pg_conn, where_clause);
  
  return 0;
}

