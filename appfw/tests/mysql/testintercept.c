#include <mysql.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int main(int argc, char *argv[]) 
{
   MYSQL *conn = NULL;

   char query[2048];
   char bogus[2048];
   char *query_data = getenv("QUERY_DATA");

   // make sure we have 1 character fragments
   sprintf(&bogus[3],"o");
   sprintf(&bogus[5],"r");

   sprintf(query,"SeLEcT * FROM someTable \n WHERE id='%s'", query_data);

   printf("query: %s\n", query);

   mysql_query(conn, query);
}

