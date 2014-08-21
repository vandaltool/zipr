#include <mysql.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) 
{
	MYSQL *conn = NULL;
	MYSQL_RES *res;
	MYSQL_ROW row;
	MYSQL_STMT stmt;

	char query[2048];
	char *query_data = getenv("QUERY_DATA");
	
	if (query_data)
	{
		sprintf(query,"SELECT * FROM someTable WHERE id='?' %s LIMIT 5", query_data);
	}
	else
	{
		sprintf(query,"SELECT * FROM someTable WHERE id='?' LIMIT 5");
	}

	printf("query: %s\n", query);

	mysql_stmt_prepare(&stmt, query, strlen(query));
}

