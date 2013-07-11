#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include <libpq-fe.h>

#include "sqlfw.h"

PGresult* (*my_pgExec)(PGconn*, const char *) = NULL;
PGresult* PQexec(PGconn* p_conn, const char *p_query)
{
  if (!my_pgExec)
  {
    my_pgExec = dlsym(RTLD_NEXT, "PQexec");
    sqlfw_init(); 
  }

  char *errMsg = NULL;
  if (sqlfw_verify(p_query, &errMsg))
  {
    	PGresult *ret = my_pgExec(p_conn, p_query);
	return ret;
  }
  else
  {
	// error policy: issue bad query on purpose so that we return what PG would have returned
    	PGresult *ret = my_pgExec(p_conn, "not a query force error");
	return ret;
  }
}

int (*my_SQLExecDirect)(void* stmt,char *query, int query_len) = NULL;
int SQLExecDirect(void* stmt,char *query, int query_len)
{
  	if (!my_SQLExecDirect)
  	{
    		my_SQLExecDirect = dlsym(RTLD_NEXT, "SQLExecDirect");
    		sqlfw_init(); 
  	}

  	char *errMsg = NULL;
  	if (sqlfw_verify(query, &errMsg))
  	{
    		int ret = my_SQLExecDirect(stmt,query,query_len);
		return ret;
  	}
  	else
  	{
		// error policy: issue bad query on purpose so that we return what PG would have returned
    		int ret = my_SQLExecDirect(stmt,"not a query force error", -3);
		return ret;
  	}
}

int (*my_SQLPrepare)(void* stmt,char* query, int query_len) = NULL;
int SQLPrepare(void* stmt,char* query, int query_len)
{
  	if (!my_SQLPrepare)
  	{
    		my_SQLPrepare = dlsym(RTLD_NEXT, "SQLPrepare");
    		sqlfw_init(); 
  	}

  	char *errMsg = NULL;
  	if (sqlfw_verify(query, &errMsg))
  	{
    		int ret = my_SQLPrepare(stmt,query,query_len);
		return ret;
  	}
  	else
  	{
		// error policy: issue bad query on purpose so that we return what PG would have returned
    		int ret = my_SQLPrepare(stmt,"not a query force error", -3);
		return ret;
  	}
}

