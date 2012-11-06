/* This software was developed at the National Institute of Standards and
 * Technology by employees of the Federal Government in the course of their
 * official duties. Pursuant to title 17 Section 105 of the United States
 * Code this software is not subject to copyright protection and is in the
 * public domain. NIST assumes no responsibility whatsoever for its use by
 * other parties, and makes no guarantees, expressed or implied, about its
 * quality, reliability, or any other characteristic.

 * We would appreciate acknowledgement if the software is used.
 * The SAMATE project website is: http://samate.nist.gov

@GOOD_ARGS David
@NORMAL_OUTPUT_CONTAINS David
@TEST_SCRIPT ./runtest
@BAD_ARGS "David' or '0'='0"
@ATTACK_SUCCEEDED_OUTPUT_CONTAINS William

*/
#include <stdlib.h>
#include <libpq-fe.h>
#include <stdio.h>
#include <string.h>


int main(int argc, char *argv[]) 
{
   PGconn *conn;
   PGresult *result;

	if (argc < 2)
	{
		printf("You should give an entry parameter!\n");
		return 0;		
	}

   char connectionString[512];
   sprintf(connectionString, "dbname=%s", getenv("PGDATABASE"));
   conn = PQconnectdb(connectionString);

  // Check to see that the backend connection was successfully made
   if (PQstatus(conn) != CONNECTION_OK)
   {
     printf("Connection to database failed.\n");
     PQfinish(conn);
     return 0;
   }
  printf("Connection to database - OK\n");
 
   /* send SQL query */
   char query[512];
   char *fmtString = "SELECT * FROM users_1796 WHERE first LIKE '%s'; -- comment ;";
   
   /* No more buffer overflow */
	if ((strlen(argv[1]) + strlen(fmtString)) > 512){
		printf("The entry is too long...\n");
		return 0;		
	}
	sprintf(query,fmtString,argv[1]);
        result = PQexec(conn, query);
        printf("%s\n", query);
	
	/* output fields 1 and 2 of each row */
        int i;
        for (i = 0; i < PQntuples(result); ++i)
        {
	  printf("%s %s\n", PQgetvalue(result,i,0), PQgetvalue(result,i,1));
        }
	
	/* Release memory used to store results and close connection */
        PQclear(result);
        PQfinish(conn);
	return 0;
}

