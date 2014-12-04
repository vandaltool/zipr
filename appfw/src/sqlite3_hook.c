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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include "sqlite3.h"
#include "sqlfw.h"

static int within_sql_hook=FALSE;

//
// Intercept sqlite queries & prepared statements
// NB: only handles UTF-8
//

int (*intercept_sqlite3Query)(sqlite3*, const char*, int (*)(void*,int,char**,char**), void *, char **) = NULL;
int sqlite3_exec(
  sqlite3 *db,                               /* An open database */
  const char *sql,                           /* SQL to be evaluated */
  int (*callback)(void*,int,char**,char**),  /* Callback function */
  void *arg,                                 /* 1st argument to callback */
  char **errmsg                              /* Error msg written here */
)
{
  if (!intercept_sqlite3Query)
  {
    intercept_sqlite3Query = dlsym(RTLD_NEXT, "sqlite3_exec");
    sqlfw_init(); 
  }

  char *errMsg = NULL;
  if (within_sql_hook || sqlfw_verify(sql, &errMsg))
  {
	int old_wsh=within_sql_hook;
	within_sql_hook=TRUE;
    	int ret= intercept_sqlite3Query(db, sql, callback, arg, errmsg);
	within_sql_hook=old_wsh;
	return ret;
  }
  else
  {
	// error policy: issue bad query on purpose so that we return what sqlite3 would have returned
	return intercept_sqlite3Query(db, "security violation", NULL, NULL, errmsg);
  }
}

int (*intercept_sqlite3Prepare)(sqlite3 *, const char *, int, sqlite3_stmt **, const char **) = NULL; 
int sqlite3_prepare(
  sqlite3 *db,            /* Database handle */
  const char *zSql,       /* SQL statement, UTF-8 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const char **pzTail     /* OUT: Pointer to unused portion of zSql */
)
{
	if (!intercept_sqlite3Prepare)
	{
		intercept_sqlite3Prepare = dlsym(RTLD_NEXT, "sqlite3_prepare");
		sqlfw_init(); 
	}

	char *errMsg = NULL;
	if (within_sql_hook || sqlfw_verify(zSql, &errMsg))
	{
        	int old_wsh=within_sql_hook;
        	within_sql_hook=TRUE;
		int ret= intercept_sqlite3Prepare(db, zSql, nByte, ppStmt, pzTail);
        	within_sql_hook=old_wsh;
		return ret;
	}
	else
	{
		// error policy: issue bad query on purpose so that we return what sqlite3 would have returned
		return intercept_sqlite3Prepare(db, "security violation", nByte, ppStmt, pzTail);
  	}
}

int (*intercept_sqlite3PrepareV2)(sqlite3 *, const char *, int, sqlite3_stmt **, const char **) = NULL; 
int sqlite3_prepare_v2(
  sqlite3 *db,            /* Database handle */
  const char *zSql,       /* SQL statement, UTF-8 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const char **pzTail     /* OUT: Pointer to unused portion of zSql */
)
{
	if (!intercept_sqlite3PrepareV2)
	{
		intercept_sqlite3PrepareV2 = dlsym(RTLD_NEXT, "sqlite3_prepare_v2");
		sqlfw_init(); 
	}

	char *errMsg = NULL;
	if (within_sql_hook || sqlfw_verify(zSql, &errMsg))
	{
        	int old_wsh=within_sql_hook;
        	within_sql_hook=TRUE;
		int ret= intercept_sqlite3PrepareV2(db, zSql, nByte, ppStmt, pzTail);
        	within_sql_hook=old_wsh;
		return ret;
	}
	else
	{
		// error policy: issue bad query on purpose so that we return what sqlite3 would have returned
		return intercept_sqlite3PrepareV2(db, "security violation", nByte, ppStmt, pzTail);
  	}
}
