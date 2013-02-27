#include <mysql.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) 
{
   MYSQL *conn = NULL;

   char query[2048];

   sprintf(query,"SELECT * FROM someTable WHERE id='%s'", argv[1]);

   printf("query: %s\n", query);

   mysql_query(conn, query);
}

