#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>
#include <assert.h>

#include <mysql.h>

#include "sqlfw.h"

// @todo: fix potential buffer overflow in error message
// @todo: use length to figure out taint buffer size 

// intercept mysql_query
int (*intercept_sqlQuery)(MYSQL*, const char *) = NULL;
int mysql_query(MYSQL* p_conn, const char *p_query)
{
  if (!intercept_sqlQuery)
    intercept_sqlQuery = dlsym(RTLD_NEXT, "mysql_query");
  assert(intercept_sqlQuery);

  sqlfw_init(); // will do this automagically later 

  char *errMsg = NULL;
  if (sqlfw_verify(p_query, &errMsg))
  {
    int ret = intercept_sqlQuery(p_conn, p_query);
	return ret;
  }
  else
  {
	// error policy: issue bad query on purpose so that we return what mysql would have returned
	return intercept_sqlQuery(p_conn, "error: security violation");
  }
}

int (*intercept_sqlRealQuery)(MYSQL*, const char *, unsigned long) = NULL;
// intercept mysql_real_query
int mysql_real_query(MYSQL* p_conn, const char *p_query, unsigned long p_length)
{
  if (!intercept_sqlRealQuery)
    intercept_sqlRealQuery = dlsym(RTLD_NEXT, "mysql_real_query");
  assert(intercept_sqlRealQuery);

  sqlfw_init(); // will do this automagically later 

  char *errMsg = NULL;
  if (sqlfw_verify(p_query, &errMsg))
  {
    int ret = intercept_sqlRealQuery(p_conn, p_query, p_length);
	return ret;
  }
  else
  {
	// error policy: issue bad query on purpose so that we return what mysql would have returned
	char errmsg[2048];
	sprintf(errmsg, "error: security violation");
	return intercept_sqlRealQuery(p_conn, errmsg, strlen(errmsg));
  }
}

// intercept mysql_stmt_prepare
int (*intercept_sqlStmtPrepare)(MYSQL_STMT *p_stmt, const char *p_stmt_str, unsigned long p_length) = NULL;
int mysql_stmt_prepare(MYSQL_STMT *p_stmt, const char *p_stmt_str, unsigned long p_length)
{
	if (!intercept_sqlStmtPrepare)
	{
		intercept_sqlStmtPrepare = dlsym(RTLD_NEXT, "mysql_stmt_prepare");
                assert(intercept_sqlStmtPrepare);
		sqlfw_init();
	}

	char *errMsg = NULL;
	if (sqlfw_verify(p_stmt_str, &errMsg))
	{
		int ret = intercept_sqlStmtPrepare(p_stmt, p_stmt_str, p_length);
		return ret;
	}
	else
	{
		// error policy: issue bad query on purpose so that we return what mysql would have returned
		char tmp[2048];
		sprintf(tmp, "error: security violation");
		return intercept_sqlStmtPrepare(p_stmt, tmp, p_length);
	}
}
