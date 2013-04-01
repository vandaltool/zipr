/* 
 * Test for false positives
 *
*/

#include <mysql.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) 
{
   MYSQL *conn;
   MYSQL_RES *res;
   MYSQL_ROW row;

   char *server = "localhost";
   char *user = "root";
   char *password = "root";
   char *database = "testdata";
	
   conn = mysql_init(NULL);
   
   /* Connect to database */
   if (!mysql_real_connect(conn, server,
         user, password, database, 0, NULL, 0)) {
      fprintf(stderr, "%s\n", mysql_error(conn));
      return 0;
   }

   /* test for false positives */
   test1(conn);

   /* cleanup */
	mysql_close(conn);
	return 0;
}

/* Test multi-statement selects */
void test1(MYSQL *conn)
{
	char query[1024];

	printf("Testing multi-statements selects\n");

	sprintf(query, "Select * FROM users_1796 LIMIT 10");
	mysql_query(conn, query);

	sprintf(query, "Select * FROM users_1796 LIMIT 10;");
	mysql_query(conn, query);

	sprintf(query, "Select * FROM users_1796 LIMIT 10 ;");
	mysql_query(conn, query);

	sprintf(query, "Select * FROM users_1796 LIMIT 10 ; ");
	mysql_query(conn, query);

	sprintf(query, "Select * FROM users_1796 whEre id='abc' LIMIT 10 ; ");
	mysql_query(conn, query);

	sprintf(query, "Select * FROM users_1796 LIMIT 10 ; SELECT * FROM users_1796 LIMIT 2 ;");
	mysql_query(conn, query);
}

