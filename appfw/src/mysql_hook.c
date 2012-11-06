#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include <mysql.h>

#include "sqlfw.h"

int mysql_query(MYSQL* p_conn, const char *p_query)
{
  char tainted[MAX_QUERY_LENGTH];
  static int (*intercept_sqlQuery)(MYSQL*, const char *) = NULL;
  if (!intercept_sqlQuery)
    intercept_sqlQuery = dlsym(RTLD_NEXT, "mysql_query");

  sqlfw_init(); // will do this automagically later 

  char *errMsg = NULL;
  if (sqlfw_verify_taint(p_query, tainted, &errMsg))
  {
    int ret = intercept_sqlQuery(p_conn, p_query);
	return ret;
  }
  else
  {
	// error policy: issue bad query on purpose so that we return what PG would have returned
	char errmsg[2048];
	sprintf(errmsg, "error: security violation: %s", p_query);
    int ret = intercept_sqlQuery(p_conn, errmsg);
	return ret;
  }
}
