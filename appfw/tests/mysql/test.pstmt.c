#include <mysql.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) 
{
	MYSQL *conn = NULL;
	MYSQL_RES *res;
	MYSQL_ROW row;

	char query[2048];
	char *query_data = getenv("QUERY_DATA");

	char *server = "localhost";
	char *user = "root";
	char *password = "root";
	char *password = NULL;
	char *database = "testdata";
	
	conn = mysql_init(NULL);
	
	/* Connect to database */
	if (!mysql_real_connect(conn, server,
	      user, password, database, 0, NULL, 0)) {
	   fprintf(stderr, "%s\n", mysql_error(conn));
	   return 0;
	}

	if (query_data)
	{
		sprintf(query,"SELECT * FROM someTable WHERE id='?' LIMIT 5");
	}
	else
	{
		sprintf(query,"SELECT * FROM someTable WHERE id='?' %s LIMIT 5", query_data);
	}

	printf("query: %s\n", query);

	MSQL_STMT *stmt = mysql_stmt_init(conn);
	mysql_stmt_prepare(stmt, query, strlen(query));
	MYSQL_BIND bind[1];
	int val = 1;
	bind[0].buffer_type = MYSQL_TYPE_LONG;
	bind[0].buffer = &val;
	bind[0].is_unsigned = 0;
	bind[0].is_null = 0;
	bind[0].length = 0;
	mysql_stmt_bind_param(stmt, bind);

	if (res = mysql_stmt_execute(stmt) != 0)
	{
		fprintf(stderr, "could not execute statement: %s\n", query);
	}

	printf("Num rows retrieved: %d\n", mysql_num_rows(res));
}

