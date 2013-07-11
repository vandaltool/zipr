#include <stdio.h>
#include <sql.h>
#include <sqlext.h>
#include <string.h>
#include <stdlib.h>

/*
 * see Retrieving ODBC Diagnostics
 * for a definition of extract_error().
 */
static void extract_error(
    char *fn,
    SQLHANDLE handle,
    SQLSMALLINT type);

main() {
  SQLHENV env;
  SQLHDBC dbc;
  SQLHSTMT stmt;
  SQLRETURN ret; /* ODBC API return status */
  SQLCHAR outstr[1024];
  SQLSMALLINT outstrlen;
  SQLSMALLINT columns; /* number of columns in result-set */
  int row = 0;



  /* Allocate an environment handle */
  SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
  /* We want ODBC 3 support */
  SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
  /* Allocate a connection handle */
  SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
  /* Connect to the DSN mydsn */

  // peasoup-datamanager.cs.virginia.edu
  // buildbot, h3llostr4ta
  // 
  ret = SQLDriverConnect(dbc, NULL, "DSN=PostgreSQL_Test;UID=an7s;PWD=h3llostr4ta", SQL_NTS,
			 outstr, sizeof(outstr), &outstrlen,
			 SQL_DRIVER_COMPLETE);

  if (SQL_SUCCEEDED(ret)) {
    printf("Connected\n");
    printf("Returned connection string was:\n\t%s\n", outstr);

    //BILLY
    char query[100]="SELECT * FROM ";


    if (getenv("QUERY")){
    	strncat(query,getenv("QUERY"),50); 
		strcat(query,"   LIMIT 10");
    }else{
	exit(1);
    }

   strcat(query,";"); 

/* Allocate a statement handle */
SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);
    //method 1 to query
    int return_code = SQLExecDirect(stmt,query,strlen(query));
    printf("return_code:%d\n",return_code); 

/* Retrieve a list of tables */
//   SQLTables(stmt, NULL, 0, NULL, 0, NULL, 0, "TABLE", SQL_NTS);
/* How many columns are there */
   SQLNumResultCols(stmt, &columns);
/* Loop through the rows in the result-set */
printf("got here 1\ncolumns is: %d\n",columns);

while (SQL_SUCCEEDED(ret = SQLFetch(stmt))) {
    SQLUSMALLINT i;
    printf("Row %d\n", row++);
    /* Loop through the columns */
    for (i = 1; i <= columns; i++) {
        SQLINTEGER indicator;
        char buf[512];
        /* retrieve column data as a string */
	ret = SQLGetData(stmt, i, SQL_C_CHAR,
                         buf, sizeof(buf), &indicator);
        if (SQL_SUCCEEDED(ret)) {
            /* Handle null columns */
            if (indicator == SQL_NULL_DATA) strcpy(buf, "NULL");
	    printf("  Column %u : %s\n", i, buf);
        }
    }
  }

    //method 2 to query
 //   SQLPrepare(stmt,query,strlen(query));
  //  SQLExecute(stmt);
    //BILLY

    if (ret == SQL_SUCCESS_WITH_INFO) {
      printf("Driver reported the following diagnostics\n");
      //extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
    }
    SQLDisconnect(dbc);		/* disconnect from driver */
  } else {
    fprintf(stderr, "Failed to connect\n");
    //extract_error("SQLDriverConnect", dbc, SQL_HANDLE_DBC);
  }
  /* free up allocated handles */
  SQLFreeHandle(SQL_HANDLE_DBC, dbc);
  SQLFreeHandle(SQL_HANDLE_ENV, env);
}
