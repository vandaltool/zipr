#include <stdio.h>
#include <stdlib.h>
#include "libpq-fe.h"

void exitNicely(PGconn *p_conn)
{
  PQfinish(p_conn);
  exit(1);
}

int main(int argc, char **argv)
{
  char conninfo[1024];
  char query[1024];
  PGconn     *conn;
  PGresult   *res;

  sprintf(conninfo, "dbname = %s", getenv("PGDATABASE"));

  conn = PQconnectdb(conninfo);

  if (PQstatus(conn) != CONNECTION_OK)
  {
    fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
	exitNicely(conn);
  }

  // deliberately allow for SQL injection
  sprintf(query, "select * from doip where comment = '%s'; select * from doip where comment = '%s'  ;", argv[1], argv[1]);
  fprintf(stderr,"issuing query: %s\n", query);
  res = PQexec(conn, query);
  PQclear(res);

  sprintf(query, "   select * from doip where comment = '%s'   ; select * FROM doip where comment = '%s'  /* --  */ ; select * from doip;", argv[1], argv[1]);
  fprintf(stderr,"issuing query: %s\n", query);
  res = PQexec(conn, query);
  PQclear(res);

  PQfinish(conn);

  return 0;
}
