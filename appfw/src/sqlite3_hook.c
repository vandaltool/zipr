#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include "sqlite3.h"

#include "sqlfw.h"

int sqlite3_exec(
  sqlite3 *db,                               /* An open database */
  const char *sql,                           /* SQL to be evaluated */
  int (*callback)(void*,int,char**,char**),  /* Callback function */
  void *arg,                                 /* 1st argument to callback */
  char **errmsg                              /* Error msg written here */
)
{
  char tainted[MAX_QUERY_LENGTH];
  static int (*intercept_sqlite3Query)(sqlite3*, const char*, int (*)(void*,int,char**,char**), void *, char **) = NULL;
  if (!intercept_sqlite3Query)
    intercept_sqlite3Query = dlsym(RTLD_NEXT, "sqlite3_exec");

  sqlfw_init(); 

#ifdef BOGUS
  // WE NOW ADD THE SIGNATURES FROM SQLITE INTO THE SET OF SIGNATURES 
  // need to whitelist: SQL Injection detected: SELECT name, rootpage, sql FROM 'main'.sqlite_master ORDER BY rowid
  // @todo: to make sure we catch all such cases, we should link against sqlite3 itself and extract its signatures
  if (strncasecmp("SELECT name", sql, strlen("SELECT name")) == 0 &&
      strcasestr(sql, "sqlite_master"))
  {
    return intercept_sqlite3Query(db, sql, callback, arg, errmsg);
  }
#endif

  char *errMsg = NULL;
  if (sqlfw_verify_taint(sql, tainted, &errMsg))
  {
    return intercept_sqlite3Query(db, sql, callback, arg, errmsg);
  }
  else
  {
#ifdef SHOW_TAINT_MARKINGS
	sqlfw_display_taint("SQL Injection detected", sql, tainted);
#endif

	// error policy: issue bad query on purpose so that we return what sqlite3 would have returned
	char badquery[2048];
	sprintf(badquery, "error: security violation: %s", sql);
    return intercept_sqlite3Query(db, badquery, NULL, NULL, errmsg);
  }
}
