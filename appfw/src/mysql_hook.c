#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include <mysql.h>

#include "sqlfw.h"

// @todo: fix potential buffer overflow in error message
// @todo: use length to figure out taint buffer size 

// intercept mysql_query
int mysql_query(MYSQL* p_conn, const char *p_query)
{
  static int (*intercept_sqlQuery)(MYSQL*, const char *) = NULL;
  if (!intercept_sqlQuery)
    intercept_sqlQuery = dlsym(RTLD_NEXT, "mysql_query");

  sqlfw_init(); // will do this automagically later 

  char *errMsg = NULL;
  if (sqlfw_verify_taint(p_query, &errMsg))
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

// intercept mysql_real_query
int mysql_real_query(MYSQL* p_conn, const char *p_query, unsigned long p_length)
{
  static int (*intercept_sqlRealQuery)(MYSQL*, const char *, unsigned long) = NULL;
  if (!intercept_sqlRealQuery)
    intercept_sqlRealQuery = dlsym(RTLD_NEXT, "mysql_real_query");

  sqlfw_init(); // will do this automagically later 

  char *errMsg = NULL;
  if (sqlfw_verify(p_query, &errMsg))
  {
    int ret = intercept_sqlRealQuery(p_conn, p_query, p_length);
	return ret;
  }
  else
  {
	// error policy: issue bad query on purpose so that we return what PG would have returned
	char errmsg[2048];
	sprintf(errmsg, "error: security violation: %s", p_query);
    int ret = intercept_sqlRealQuery(p_conn, errmsg, strlen(errmsg));
	return ret;
  }
}
