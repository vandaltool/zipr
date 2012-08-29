#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include <libpq-fe.h>

#include "sqlfw.h"

PGresult* PQexec(PGconn* p_conn, const char *p_query)
{
  char taint[MAX_QUERY_LENGTH];
  static PGresult* (*my_pgExec)(PGconn*, const char *) = NULL;
  if (!my_pgExec)
    my_pgExec = dlsym(RTLD_NEXT, "PQexec");

  sqlfw_init(); // will do this automagically later 

  char *errMsg = NULL;
  if (sqlfw_verify_taint(p_query, taint, &errMsg))
  {
    PGresult *ret = my_pgExec(p_conn, p_query);
	return ret;
  }
  else
  {

    // show query and taint markings
//    fprintf(stderr, "[appfw]: PQexec(): SQL Command Injection detected in query: %s\n", p_query);

#ifdef SHOW_TAINT_MARKINGS
    sqlfw_display_taint("SQL Injection detected", p_query, taint);
	/*
    fprintf(stderr, "[appfw]:                                         taint markings: ");
    for (i = 0; i < strlen(p_query); ++i)
    {
      if (taint[i] == APPFW_BLESSED)
        fprintf(stderr," ");
      else if (taint[i] == APPFW_SECURITY_VIOLATION)
        fprintf(stderr,"X");
      else
        fprintf(stderr,"-");

    }
    fprintf(stderr,"\n");
	*/
#endif

	// error policy: issue bad query on purpose so that we return what PG would have returned
    PGresult *ret = my_pgExec(p_conn, "not a query force error");
	return ret;
  }
}
