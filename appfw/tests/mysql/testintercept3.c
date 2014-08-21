#include <mysql.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[]) 
{
   MYSQL *conn = NULL;

   char query[2048];
   char *query_data = getenv("QUERY_DATA");


   sprintf(query,"SELECT * FROM someTable WHERE myId=%s", query_data);

   printf("query: %s\n", query);

fprintf(stderr,"\n>>> issuing query\n");
   mysql_real_query(conn, query, strlen(query));
fprintf(stderr,">>> done issuing query\n\n");
}

