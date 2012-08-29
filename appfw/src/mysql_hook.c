#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include <mysql.h>

#include "sqlfw.h"

int mysql_query(MYSQL* p_conn, const char *p_query)
{
  static int (*intercept_sqlQuery)(MYSQL*, const char *) = NULL;
  if (!intercept_sqlQuery)
    intercept_sqlQuery = dlsym(RTLD_NEXT, "mysql_query");

  sqlfw_init(); // will do this automagically later 

  char *errMsg = NULL;
  if (sqlfw_verify(p_query, &errMsg))
  {
    int ret = intercept_sqlQuery(p_conn, p_query);
	return ret;
  }
  else
  {
    int i;
    char taint[strlen(p_query)+32];
    sqlfw_establish_taint(p_query, taint);

    // show query and taint markings
    fprintf(stderr, "[appfw]: mysql_query(): SQL Command Injection detected in query: %s\n", p_query);

#ifdef SHOW_TAINT_MARKINGS
    fprintf(stderr, "[appfw]:                                         taint markings: ");
	for (i = 0; i < strlen(p_query); ++i)
	{
      if (taint[i])
	    fprintf(stderr,"^");
	  else
	    fprintf(stderr,"-");
	}
	fprintf(stderr,"\n");
#endif

	// error policy: issue bad query on purpose so that we return what PG would have returned
    int ret = intercept_sqlQuery(p_conn, "not a query force error");
	return ret;
  }
}
