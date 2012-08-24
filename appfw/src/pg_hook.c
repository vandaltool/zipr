#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>

#include <libpq-fe.h>

#include "sqlfw.h"

PGresult* PQexec(PGconn* p_conn, const char *p_query)
{
  static PGresult* (*my_pgExec)(PGconn*, const char *) = NULL;
  if (!my_pgExec)
    my_pgExec = dlsym(RTLD_NEXT, "PQexec");

  sqlfw_init(); // will do this automagically later 

  char *errMsg = NULL;
  if (sqlfw_verify(p_query, &errMsg))
  {
    PGresult *ret = my_pgExec(p_conn, p_query);
//    fprintf(stderr, "PQexec(): intercepted: query: %s\n", p_query);
	return ret;
  }
  else
  {
    fprintf(stderr, "[appfw]: PQexec(): SQL Command Injection detected in query: %s\n", p_query);
    PGresult *ret = my_pgExec(p_conn, "not a query force error");
	return ret;
  }
}
