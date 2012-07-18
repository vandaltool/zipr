#include <stdio.h>
#include "sqlite3.h"

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
fprintf(stderr,"\n--callback function:\n");
  int i;
  for(i=0; i<argc; i++){
    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}


void test_query(sqlite3 *db, char *userid, char *userpassword)
{
  int i, rc;
  char *zErrMsg = 0;
  char query[1024];

 // sprintf(query, "SELECT * from users where id='%s' and password='%s'", userid, userpassword);
  sprintf(query, "SELECT * from users where id='%s'", userid, userpassword);

/*
  char *select="SELECT * from users where id='";
  char *and="' and password='";
  sprintf(query, "%s%s%s%s'  ; select * from users;", select, userid, and, userpassword);
 */

  fprintf(stderr,"query: %s\n", query);

  sqlfw_verify(query, &zErrMsg);

  rc = sqlite3_exec(db, query, callback, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
}


// DB: testdb
// table: users
// fields: name, password
//         john, john
int main(int argc, char **argv){
  sqlite3 *db;
  char *zErrMsg = 0;
  int rc;

  sqlfw_init("peasoup", argv[1]); // argv[1] is the signature file

  rc = sqlite3_open("testdb", &db);
  if( rc ){
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return(1);
  }

  char *id = argv[2];
  char *password = argv[3];

  test_query(db, id, password);

  sqlite3_close(db);
  return 0;
}

// ------------------------------------------
