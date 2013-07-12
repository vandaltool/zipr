#include <stdio.h>
#include <stdlib.h>
#include <postgres.h>
#include <libpq-fe.h>
#include <catalog/pg_type.h>

// test prepared statements
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
  char *query_data = getenv("QUERY_DATA");

  sprintf(conninfo, "dbname = %s", getenv("PGDATABASE"));

  conn = PQconnectdb(conninfo);

  if (PQstatus(conn) != CONNECTION_OK)
  {
    fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
	exitNicely(conn);
  }

  // deliberately allow for SQL injection
  sprintf(query, "select * from file_info WHERE file_id = $1 LIMIT 5");
  fprintf(stderr,"issuing query: %s\n", query);

  Oid oids[1];
  oids[0] = INT4OID; // TEXTOID, BOOLOID, INT4OID, ...
  PQprepare(conn, "stmt", query, 1, oids);

  char *vals[1];  vals[0] = query_data;
  int lengths[1]; lengths[0] = strlen(query_data);
  int formats[1]; formats[0] = 0; // text format
  res = PQexecPrepared(conn, "stmt", 1, vals, lengths, formats, 0);
  if (PQresultStatus(res) == PGRES_TUPLES_OK)
  {
    fprintf(stderr, "-------------------------------------------\n");
    fprintf(stderr, "\tQuery success: %s\n", query);
    fprintf(stderr, "-------------------------------------------\n");

	int numRows = PQntuples(res);
	fprintf(stderr,"%d rows retrieved\n", numRows);
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
