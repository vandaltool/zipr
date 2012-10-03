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
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  for(i=0; i<argc; i++){
    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}

int main(int argc, char *argv[]) 
{
   sqlite3 *db;
   char *database = "testdata";
   int rc;
   char *zErrMsg = 0;
   
   if (argc < 2)
   {
       printf("You should give an entry parameter!\n");
       return 0;		
   }
	
   /* Open Database */
   rc = sqlite3_open(database, &db);
   if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return(1);
    }

   /* create SQL query */
   char query[512];
   char *fmtString = "SELECT * FROM users_1796 WHERE first LIKE '%s'";
   
   /* No more buffer overflow */
   if ((strlen(argv[1]) + strlen(fmtString)) > 512){
	printf("The entry is too long...\n");
	return 0;		
   }
   sprintf(query,fmtString,argv[1]);
   printf("%s\n", query);

   /* Execute Query */
   rc = sqlite3_exec(db, query, callback, 0, &zErrMsg);
   if( rc!=SQLITE_OK ){
     fprintf(stderr, "SQL error: %s\n", zErrMsg);
     sqlite3_free(zErrMsg);
   }
	
   /* Release memory used to store results and close connection */
   sqlite3_close(db);
   return 0;
}

