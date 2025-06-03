#include "Persons.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void InsertPersons(PGconn* conn, struct Persons u) {
  char query[1024];
  snprintf(query, sizeof(query),
    "INSERT INTO Persons (personid, lastname, firstname, address, city) "
    "VALUES ('%s', '%s', '%s', '%s', '%s');",
    u.personid, u.lastname, u.firstname, u.address, u.city);
  PGresult* res = PQexec(conn, query);
  PQclear(res);
}

void UpdatePersons(PGconn* conn, struct Persons u, const char* where_clause) {
  char query[1024];
  snprintf(query, sizeof(query),
    "UPDATE Persons "
    "SET personid='%s', lastname='%s', firstname='%s', address='%s', city='%s' WHERE %s;",
    u.personid, u.lastname, u.firstname, u.address, u.city, where_clause);
  PGresult* res = PQexec(conn, query);
  PQclear(res);
}

void DeletePersons(PGconn* conn, const char* where_clause) {
  char query[512];
  snprintf(query, sizeof(query),
    "DELETE FROM Persons WHERE %s;",
    where_clause);
  PGresult* res = PQexec(conn, query);
  PQclear(res);
}
