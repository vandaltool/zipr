#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include "sqlite3.h"
#include "sqlfw.h"

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
  if (sqlfw_verify(sql, &errMsg))
  {
    return intercept_sqlite3Query(db, sql, callback, arg, errmsg);
  }
  else
  {
	// error policy: issue bad query on purpose so that we return what sqlite3 would have returned
	char badquery[2048];
	sprintf(badquery, "error: security violation: %s", sql);
    return intercept_sqlite3Query(db, badquery, NULL, NULL, errmsg);
  }
}
