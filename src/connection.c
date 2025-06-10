#include <pog_pool/connection.h>

void InitPool(volatile ConnectionPool *pool, const char *conninfo)
{
  for (int i = 0; i < MAX_CONNECTIONS; i++)
  {
    pool->connections[i] = NULL;
  }
  pool->num_connections = 0;
  
  for (int i = 0; i < MAX_CONNECTIONS; i++)
  {
    pool->connections[i] = PQconnectdb(conninfo);
    if (PQstatus(pool->connections[i]) != CONNECTION_OK)
    {
      printf("Connection to database failed: %s\n", PQerrorMessage(pool->connections[i]));
      exit(1);
    }
    pool->num_connections++;
  }
}

PGconn *BorrowConnection(volatile ConnectionPool *pool)
{
  if (pool->num_connections == 0)
  {
    printf("No available connections in the pool.\n");
    return NULL;
  }
  return pool->connections[--pool->num_connections];
}

void ReleaseConnection(volatile ConnectionPool *pool, PGconn *conn)
{
  if (pool->num_connections >= MAX_CONNECTIONS)
  {
    printf("Pool is full. Cannot release connection.\n");
    return;
  }
  pool->connections[pool->num_connections++] = conn;
}

void ClosePool(volatile ConnectionPool *pool)
{
  for (int i = 0; i < pool->num_connections; i++)
  {
    PQfinish(pool->connections[i]);
  }
  pool->num_connections = 0;
}


void RunSQLFile(PGconn *conn, const char *filename)
{
  FILE *file = fopen(filename, "r");
  if (!file)
  {
    perror("Failed to open SQL file");
    exit(EXIT_FAILURE);
  }

  fseek(file, 0, SEEK_END);
  long length = ftell(file);
  rewind(file);

  char *query = malloc(length + 1);
  fread(query, 1, length, file);
  query[length] = '\0';
  fclose(file);

  PGresult *res = PQexec(conn, query);
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    fprintf(stderr, "SQL execution failed: %s\n", PQerrorMessage(conn));
    PQclear(res);
    free(query);
    exit(EXIT_FAILURE);
  }

  PQclear(res);
  free(query);
}
