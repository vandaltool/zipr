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
  char *queryfile = argv[1];
  char *querydata = argv[2];
  FILE *queryfd;
  PGconn     *conn;
  PGresult   *res;

  sprintf(conninfo, "dbname = %s", getenv("PGDATABASE"));

  conn = PQconnectdb(conninfo);

  if (PQstatus(conn) != CONNECTION_OK)
  {
    fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
	exitNicely(conn);
  }

  queryfd = fopen(queryfile, "r+");
  
  fgets(query, 1000, queryfd);
  // deliberately allow for SQL injection
  sprintf(query, "%s '%s'", query, querydata);
  fprintf(stderr,"issuing query: %s\n", query);
  res = PQexec(conn, query);
  if (PQresultStatus(res) == PGRES_TUPLES_OK)
  {
    fprintf(stderr, "-------------------------------------------\n");
    fprintf(stderr, "\tQuery success: %s\n", query);
    fprintf(stderr, "-------------------------------------------\n");
  }
  else
  {
    fprintf(stderr, "-------------------------------------------\n");
    fprintf(stderr, "\tQuery failed: %s\n", query);
    fprintf(stderr, "-------------------------------------------\n");
  }

  PQclear(res);

  PQfinish(conn);

  return 0;
}
