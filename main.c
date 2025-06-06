#include "pog_pool/connection.h"
#include "pog_pool/auto_generate.h"
#include "build/models.h"


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

volatile ConnectionPool* connection_pool;

int main()
{
  const char *config_path = "pog_pool_config.yml";
  PogPoolConfig pogPoolConfig = {0};

  ParseConfig(config_path, &pogPoolConfig);

  ConnectionPool connection_pool_real={0};
  connection_pool = &connection_pool_real;

  InitPool(connection_pool, pogPoolConfig.database_url);
  PGconn *pg_conn = BorrowConnection(connection_pool);

  PersonsQuery p = QueryPersons(pg_conn, "1=1");
  printf("%s\n", p.Persons->personid);

  ExampleTableQuery et = {0};
  et = QueryExampleTable(pg_conn, "1=1");
  if (et.ExampleTable != NULL)
  {
    printf("%s\n", et.ExampleTable->id);
  }
  return 0;
}
