/*
 * Copyright (c) 2013, 2014 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

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
  char *querydata;
  char bogus[1024];
  PGconn     *conn;
  PGresult   *res;

  sprintf(conninfo, "dbname = %s", getenv("PGDATABASE"));

  conn = PQconnectdb(conninfo);

  if (PQstatus(conn) != CONNECTION_OK)
  {
    fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
	exitNicely(conn);
  }

  querydata = getenv("QUERY_DATA");
  // deliberately allow for SQL injection
  // test multi-line sql statements
  sprintf(query, "select * from doip -- first part of the string\n where comment -- 2nd part of string\n   = '%s';", querydata);
  fprintf(stdout, "issuing query: %s\n", query);

  // force signatures to contain the letter o,r,a,n,d to make sure
  // we don't allow AND OR to be made up of single letters
  sprintf(bogus,"o");
  sprintf(&bogus[1],"r");
  sprintf(&bogus[2],"=");
  sprintf(&bogus[3],";");
  sprintf(&bogus[4],"-");
  sprintf(&bogus[5],"a");
  sprintf(&bogus[6],"n");
  sprintf(&bogus[7],"d");

  // play with uppercase in SQL instructions (SQL is case insensitive)
  query[0]='S';
  query[2]='L';
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
