#include "pog_pool/connection.h"
#include "pog_pool/auto_generate.h"
#include "../build/models.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

volatile ConnectionPool *connection_pool;

int main()
{
  const char *config_path = "./example/pog_pool_config.yml";
  PogPoolConfig pogPoolConfig = {0};

  ParseConfig(config_path, &pogPoolConfig);

  ConnectionPool connection_pool_real = {0};
  connection_pool = &connection_pool_real;

  InitPool(connection_pool, pogPoolConfig.database_url);
  PGconn *pg_conn = BorrowConnection(connection_pool);
  RunSQLFile(pg_conn, "models/ExampleTable.sql");

  // This clean up is needed incase test failed then we need to delete it again.
  DeleteExampleTable(pg_conn, "id='b6d4c431-f327-4a4a-9345-320aa3cd7e31'");

  ExampleTable example = {
    .id = "b6d4c431-f327-4a4a-9345-320aa3cd7e31",
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
    .int_array_col = (int[]){1, 2, 3},
    .int_array_col_len = 3,
    .text_array_col = (char*[]){"apple", "banana"},
    .text_array_col_len = 2,
    .status_col = "active",
    .file_col = "68656c6c6f"
  };

  // Insert
  ExecStatusType insert_status = InsertExampleTable(pg_conn, example).status;
  assert(insert_status != PGRES_FATAL_ERROR && "Insert failed");

  // Query
  ExampleTableQuery et = QueryExampleTable(pg_conn, "*", "1=1");
  assert(et.ExampleTable != NULL);
  ExampleTable *e = et.ExampleTable;

  // Assert values
  assert(strcmp(e->id, example.id) == 0);
  assert(e->int_col == 42);
  assert(e->big_int_col == 9000000000LL);
  printf("%s   char_data\n", e->char_col);
  assert(strncmp(e->char_col, "char_data", 9) == 0);
  assert(strncmp(e->text_array_col[1], "banana", 6) == 0);
  assert(e->int_array_col[0] == 1);

  // Update
  e->char_col = "updated";
  ExecStatusType update_status = UpdateExampleTable(pg_conn, *e, "id='b6d4c431-f327-4a4a-9345-320aa3cd7e31'").status;
  assert(update_status != PGRES_FATAL_ERROR && "Update failed");

  // Query updated value
  ExampleTableQuery updated = QueryExampleTable(pg_conn, "char_col", "id='b6d4c431-f327-4a4a-9345-320aa3cd7e31'");
  assert(strncmp(updated.ExampleTable->char_col, "updated", 7) == 0);

  // Serialize
  char *json = SerializeExampleTable(*e);
  assert(json && "Serialization failed");
  printf("Serialized: %s\n", json);

  // Cleanup
  char where_clause[128];
  snprintf(where_clause, sizeof(where_clause), "id='%s'", example.id);
  DeleteExampleTable(pg_conn, where_clause);

  printf("✅ All tests passed\n");
  return 0;
}
