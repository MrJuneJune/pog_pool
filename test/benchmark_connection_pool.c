#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <postgresql/libpq-fe.h>
#include "pog_pool/connection.h"

#define N_QUERIES 1000

// This can be from 
const char *CONNINFO = "postgres://pog_pool:pog_pool@localhost:4269/pog_pool";

double NowSeconds()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec / 1e9;
}

void RunQuery(PGconn *conn)
{
  PGresult *res = PQexec(conn, "SELECT *FROM ExampleTable");
  PQclear(res);
}

void TestWithPool(ConnectionPool pool)
{
  double start = NowSeconds();
  for (int i = 0; i < N_QUERIES; i++)
  {
    PGconn *conn = BorrowConnection(&pool);
    RunQuery(conn);
    ReleaseConnection(&pool, conn);
  }
  double end = NowSeconds();
  // printf("Using pool: %.4f seconds for %d queries\n", end - start, N_QUERIES);
}

void TestWithoutPool()
{
  double start = NowSeconds();
  for (int i = 0; i < N_QUERIES; i++)
  {
    PGconn *conn = PQconnectdb(CONNINFO);
    if (PQstatus(conn) != CONNECTION_OK)
    {
      fprintf(stderr, "Connection failed: %s\n", PQerrorMessage(conn));
      PQfinish(conn);
      exit(1);
    }
    RunQuery(conn);
    PQfinish(conn);
  }
  double end = NowSeconds();

  printf("Without pool: %.4f seconds for %d queries\n", end - start, N_QUERIES);
}

int main()
{
  ConnectionPool pool;
  InitPool(&pool, CONNINFO);

  double total = 0.0;
  int n_loops = 10;
  for (int i = 0; i < n_loops; i++)
  {
    double start = NowSeconds();
    TestWithPool(pool);  // Will print each run if you leave the print inside TestWithPool
    double end = NowSeconds();
    total += (end - start);
  }

  printf("Average time over %d runs: %.4f ms\n", n_loops, (total / (n_loops *N_QUERIES) * 1000));

  ClosePool(&pool);

  // Added in case we are curious about why pool is needed
  // TestWithoutPool();
  return 0;
}

