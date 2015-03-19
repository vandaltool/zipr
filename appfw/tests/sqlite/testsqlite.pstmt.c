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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sqlite3.h"

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
   char *query_data = getenv("QUERY_DATA");
   
   /* Open Database */
   rc = sqlite3_open(database, &db);
   if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return(1);
    }

   /* create SQL query */
   char query[512];
   if (query_data)
     sprintf(query, "SELECT * FROM users_1796 WHERE first LIKE :first %s ;", query_data);
   else
     sprintf(query, "SELECT * FROM users_1796 WHERE first LIKE :first;");

   printf("%s\n", query);

   sqlite3_stmt *stmt;
   if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK)
   {
      fprintf(stderr,"error preparing stmt\n");
      exit(1);
   }

   int index = sqlite3_bind_parameter_index(stmt, ":first");
   sqlite3_bind_text(stmt, index, "David", -1, SQLITE_STATIC);

   sqlite3_step(stmt);

     char *first = sqlite3_column_text(stmt, 0);
     char *last = sqlite3_column_text(stmt, 1);
     char *balance = sqlite3_column_text(stmt, 2);
     printf("[%s] [%s] [%s]\n", first, last, balance);

   /* Release memory used to store results and close connection */
   sqlite3_close(db);
   return 0;
}

