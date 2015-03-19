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
   char query1[512];
   sprintf(query1, "SELECT * FROM users_1796 WHERE first LIKE :first;");
   char query2[512];
   sprintf(query2, "SELECT * FROM users_1796 WHERE first LIKE $1 ;");

   printf("%s\n", query1);

   sqlite3_stmt *stmt1;
   sqlite3_stmt *stmt2;
   sqlite3_stmt *stmt3;

   sqlite3_prepare_v2(db, query1, -1, &stmt1, NULL);
   sqlite3_prepare_v2(db, query2, -1, &stmt2, NULL);

   int index = sqlite3_bind_parameter_index(stmt1, ":first");

   sqlite3_bind_text(stmt1, index, "David", -1, SQLITE_STATIC);
   sqlite3_step(stmt1);
   char *first = sqlite3_column_text(stmt1, 0);
   char *last = sqlite3_column_text(stmt1, 1);
   char *balance = sqlite3_column_text(stmt1, 2);
   printf("%d: [%s] [%s] [%s]\n", index, first, last, balance);
   sqlite3_reset(stmt1);
   sprintf(first,"");

   sqlite3_bind_text(stmt2, index, "David", -1, SQLITE_STATIC);
   sqlite3_step(stmt2);
   first = sqlite3_column_text(stmt2, 0);
   last = sqlite3_column_text(stmt2, 1);
   balance = sqlite3_column_text(stmt2, 2);
   printf("%d: [%s] [%s] [%s]\n", index, first, last, balance);
   sqlite3_reset(stmt2);
   sprintf(first,"");

   char query3[512];
   sprintf(query3, "INSERT INTO users_1796(first, last) VALUES ( ?, ?);");
   sqlite3_prepare_v2(db, query3, -1, &stmt3, NULL);
   sqlite3_bind_text(stmt3, 1, "John", -1, SQLITE_STATIC);
   sqlite3_bind_text(stmt3, 2, "Doe", -1, SQLITE_STATIC);
   sqlite3_step(stmt3);

   /* Release memory used to store results and close connection */
   sqlite3_close(db);
   return 0;
}

