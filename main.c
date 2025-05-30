#include "connection.h"

const char* ExecStatusTypeToString(ExecStatusType status)
{
  switch (status)
  {
    case PGRES_EMPTY_QUERY: return "PGRES_EMPTY_QUERY";
    case PGRES_COMMAND_OK: return "PGRES_COMMAND_OK";
    case PGRES_TUPLES_OK: return "PGRES_TUPLES_OK";
    case PGRES_COPY_OUT: return "PGRES_COPY_OUT";
    case PGRES_COPY_IN: return "PGRES_COPY_IN";
    case PGRES_BAD_RESPONSE: return "PGRES_BAD_RESPONSE";
    case PGRES_NONFATAL_ERROR: return "PGRES_NONFATAL_ERROR";
    case PGRES_FATAL_ERROR: return "PGRES_FATAL_ERROR";
    case PGRES_COPY_BOTH: return "PGRES_COPY_BOTH";
    case PGRES_SINGLE_TUPLE: return "PGRES_SINGLE_TUPLE";
    case PGRES_PIPELINE_SYNC: return "PGRES_PIPELINE_SYNC";
    case PGRES_PIPELINE_ABORTED: return "PGRES_PIPELINE_ABORTED";
    default: return "Unknown ExecStatusType";
  }
}

int main()
{

  ConnectionPool *connection_pool;

  InitPool(connection_pool, "REMOVED_DB_URL");
  PGconn *pg_conn = BorrowConnection(connection_pool);

  PGresult *pg_result = PQexec(pg_conn, "select * from Persons;");
  ExecStatusType status = PQresultStatus(pg_result);

  // Get the number of rows and columns in the result
  int nrows = PQntuples(pg_result);
  int ncols = PQnfields(pg_result);

  printf("Query returned %d rows and %d columns\n", nrows, ncols);

  for (int row = 0; row < nrows; row++) {
    printf("Row %d:\n", row+1);
    for (int col = 0; col < ncols; col++) {
      char *value = PQgetvalue(pg_result, row, col);
      printf("\tColumn %d: %s\n", col + 1, value);
    }
  }

  printf("Status: %s\n", ExecStatusTypeToString(status));

  ReleaseConnection(connection_pool, pg_conn);

  printf("Released \n");

  ClosePool(connection_pool);

  printf("Closed \n");
  return 0;
}
