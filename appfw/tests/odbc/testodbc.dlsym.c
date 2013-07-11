#include <stdio.h>
#include <sql.h>
#include <sqlext.h>
#include <dlfcn.h>
 
int main() {
	SQLHENV env;
	SQLHDBC dbc;
	SQLHSTMT V_OD_hstmt;   // Handle for a statement
	SQLINTEGER V_OD_err,V_OD_id,V_OD_id2;
	long res;     // result of functions
	char *querydata = getenv("QUERY_DATA");
	char query[2048];

	fprintf(stderr,"TESTING ODBC using explicit dlopen/dlsym\n");

	SQLRETURN (*my_SQLExecDirect)(SQLHSTMT stmt, SQLCHAR *, SQLINTEGER) = 0L;
	SQLRETURN (*my_SQLConnect)(SQLHDBC, SQLCHAR *, SQLINTEGER,
	                                    SQLCHAR *, SQLINTEGER,
										SQLCHAR *, SQLINTEGER) = 0L;

 
 // /usr/lib/i386-linux-gnu/libodbc.so
 	void *libodbc = dlopen("/usr/lib/i386-linux-gnu/libodbc.so", RTLD_LAZY);
	if (!libodbc)
	{
		fprintf(stderr, "couldn't find libodbc.so: exiting");
		exit(-1);
	}

	my_SQLExecDirect = dlsym(libodbc, "SQLExecDirect");
	if (!my_SQLExecDirect)
	{
		fprintf(stderr, "couldn't resolve SQLExecDirect: exiting");
		exit(-1);
	}

	my_SQLConnect = dlsym(libodbc, "SQLConnect");
	if (!my_SQLConnect)
	{
		fprintf(stderr, "couldn't resolve SQLConnect: exiting");
		exit(-1);
	}

	// Environment
	// Allocation
	SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
 
	// ODBC: Version: Set
	SQLSetEnvAttr( env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
 
	// DBC: Allocate
	SQLAllocHandle( SQL_HANDLE_DBC, env, &dbc);
 
	// DBC: Connect
	res = (*my_SQLConnect) ( dbc, (SQLCHAR*) "PostgreSQL_Test", SQL_NTS,
	                              (SQLCHAR*) "an7s", SQL_NTS,
	                              (SQLCHAR*) "h3llostr4ta", SQL_NTS);
 
	printf("RES: %d \n", res);
 
 // table file_info
 //   file_id is a number
 //   orig_file_id is also a number
 // query:
 //     SELECT file_id, orig_file_id from file_info LIMIT 1;

	SQLAllocHandle(SQL_HANDLE_STMT, dbc, &V_OD_hstmt);
	SQLBindCol(V_OD_hstmt,1,SQL_C_ULONG,&V_OD_id,sizeof(V_OD_id),&V_OD_err);
	SQLBindCol(V_OD_hstmt,2,SQL_C_ULONG,&V_OD_id2,sizeof(V_OD_id2),&V_OD_err);

	sprintf(query,"SELECT file_id,orig_file_id FROM file_info where file_id='1' LIMIT 1;");

	if (querydata)
	{
		sprintf(query,"SELECT file_id,orig_file_id FROM file_info where file_id='%s' LIMIT 1;", querydata);
	}

	fprintf(stderr,"executing query: [%s]\n", query);
	res=(*my_SQLExecDirect)(V_OD_hstmt, query, strlen(query));
	fprintf(stderr,"query error code = %d\n", res);

	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
    {
		fprintf(stderr,"query failed\n");
		exit(0);
    }
  
	SQLDisconnect( dbc );
	SQLFreeHandle( SQL_HANDLE_DBC, dbc );
	SQLFreeHandle( SQL_HANDLE_ENV, env );
	SQLFreeHandle( SQL_HANDLE_ENV, V_OD_hstmt );
 
	return 0;
}
