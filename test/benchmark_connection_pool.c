#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <postgresql/libpq-fe.h>
#include "connection.h"

#define N_QUERIES 1000

const char *CONNINFO = "REMOVED_DB_URL";

double NowSeconds() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec / 1e9;
}

void RunQuery(PGconn *conn) {
  PGresult *res = PQexec(conn, "select * from Persons;");
  PQclear(res);
}

void TestWithPool() {
  ConnectionPool pool;
  InitPool(&pool, CONNINFO);

  double start = NowSeconds();
  for (int i = 0; i < N_QUERIES; i++) {
    PGconn *conn = BorrowConnection(&pool);
    RunQuery(conn);
    ReleaseConnection(&pool, conn);
  }
  double end = NowSeconds();
  ClosePool(&pool);

  printf("Using pool: %.4f seconds for %d queries\n", end - start, N_QUERIES);
}

void TestWithoutPool() {
  double start = NowSeconds();
  for (int i = 0; i < N_QUERIES; i++) {
    PGconn *conn = PQconnectdb(CONNINFO);
    if (PQstatus(conn) != CONNECTION_OK) {
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

int main() {
  TestWithPool();
  TestWithoutPool();
  return 0;
}

