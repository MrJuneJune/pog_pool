#ifndef Persons_STRUCT_H
#define Persons_STRUCT_H

#include <postgresql/libpq-fe.h>

struct Persons {
  char* personid;
  char* lastname;
  char* firstname;
  char* address;
  char* city;
};

void InsertPersons(PGconn* conn, struct Persons u);
void UpdatePersons(PGconn* conn, struct Persons u, const char* where_clause);
void DeletePersons(PGconn* conn, const char* where_clause);
#endif // Persons_STRUCT_H
