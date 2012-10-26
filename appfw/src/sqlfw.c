//
// Detection of SQL Injections using really simple heuristic:
//   (a) extract strings in binary
//   (b) treat all extracted strings as trusted strings
//   (c) verify SQL query for potential injections by looking for
//       SQL tokens in untrusted portion of query
//
// Reuse appfw_sqlite3 parser for parsing query strings
//

#include "appfw.h"
#include "sqlfw.h"

static const char *dbPathEnv = "APPFW_DB";
static appfw_sqlite3 *peasoupDB = NULL;
static int sqlfw_initialized = 0;

// read in signature file
// environment variable specifies signature file location
void sqlfw_init()
{
  if (sqlfw_isInitialized()) return;

  appfw_init();

  if (appfw_isInitialized() && getenv(dbPathEnv))
  {
    if (appfw_sqlite3_open(getenv(dbPathEnv), &peasoupDB) == SQLITE_OK)
      sqlfw_initialized = 1;
  }
}

// returns whether initialized
int sqlfw_isInitialized()
{
  return sqlfw_initialized;
}

void sqlfw_establish_taint(const char *query, char *taint)
{
  char **fw_sigs = appfw_getSignatures();

  if (!fw_sigs || !sqlfw_isInitialized())
    return;

  appfw_establish_taint(query, taint);
}

/*
** Run the original sqlite parser on the given SQL string.  
** Look for tainted SQL tokens/keywords
*/
int sqlfw_verify(const char *zSql, char **pzErrMsg){
  char tainted[MAX_QUERY_LENGTH];
  return sqlfw_verify_taint(zSql, tainted, pzErrMsg);
}

/*
** Run the original sqlite parser on the given SQL string.  
** Look for tainted SQL tokens/keywords
** Sets the taint array
*/
int sqlfw_verify_taint(const char *zSql, char *p_taint, char **pzErrMsg){
  Parse *pParse;
  int nErr = 0;                   /* Number of errors encountered */
  int i;                          /* Loop counter */
  void *pEngine;                  /* The LEMON-generated LALR(1) parser */
  int tokenType;                  /* type of the next token */
  int lastTokenParsed = -1;       /* type of the previous token */
  int mxSqlLen = MAX_QUERY_LENGTH;            /* Max length of an SQL string */
  int j, k;
  int beg, end;

  if (!peasoupDB)
    return 0;

  sqlfw_establish_taint(zSql, p_taint);

//  pParse = appfw_sqlite3StackAllocZero(db, sizeof(*pParse));
  pParse = appfw_sqlite3MallocZero(sizeof(*pParse));

  pParse->db = peasoupDB;

//  fprintf(stderr, "appfw_sqlite3_fw(): enter: query string length: %d\n", strlen(zSql));

  // show query
  /*
  fprintf(stderr, "[appfw]: %s\n", zSql);

  fprintf(stderr, "[appfw]: ");

  // show taint
  for (i = 0; i < strlen(zSql); ++i)
    if (p_taint[i])
      fprintf(stderr, "X");
	else
      fprintf(stderr, "-");
  fprintf(stderr,"\n");
  */

  pParse->rc = SQLITE_OK;
  pParse->zTail = zSql;
  i = 0;
  pEngine = appfw_sqlite3ParserAlloc((void*(*)(size_t))appfw_sqlite3Malloc);
  if( pEngine==0 ){
    fprintf(stderr,"Failed to allocated space for pEngine\n");
    return 0;
  }

  while( zSql[i]!=0 ){
    pParse->sLastToken.z = &zSql[i];
    pParse->sLastToken.n = appfw_sqlite3GetToken((unsigned char*)&zSql[i],&tokenType);

	beg = i;
    i += pParse->sLastToken.n;
	end = i-1;

    if( i>mxSqlLen ){
      pParse->rc = SQLITE_TOOBIG;
      break;
    }

    switch( tokenType ){
      case TK_SPACE: {
//	    fprintf(stderr, "token: [ ] type: %d [%d..%d]\n", tokenType, beg, end);
        break;
      }
      case TK_ILLEGAL: {
        goto abort_parse;
      }
      case TK_SEMI: {
        pParse->zTail = &zSql[i];
		if (p_taint[i])
		{
//          fprintf(stderr,"TAINTED SEMI-COLON DETECTED -- VERY BAD\n");
          p_taint[i] = APPFW_SECURITY_VIOLATION;
		  goto abort_parse;
		}

        /* Fall thru into the default case */
      }
      default: {
	  // show token info
	  /*
        fprintf(stderr, "token: [");
        for (k = beg; k <= end; ++k)
		  fprintf(stderr,"%c", zSql[k]);
		fprintf(stderr, "] type: %d  [%d..%d]\n", tokenType, beg, end);
		*/

        appfw_sqlite3Parser(pEngine, tokenType, pParse->sLastToken, pParse);

        lastTokenParsed = tokenType;
        if( pParse->rc!=SQLITE_OK ){
    		fprintf(stderr,"fwsql_verify(): Error in parsing:"); 
          goto abort_parse;
        }

// GOOD:
//
// SELECT * FROM foobar where one='3' and two=20;
// --------------------------------X----------XX  (taint markings)
//
// BAD:
// SELECT * FROM foobar where one='' OR 1=1; -- and two=20
// ----------------------------------XXXXXX--XX---------XX
//
// This would be detected b/c:
//   OR is p_taint
//   -- is p_taint
//   = is p_taint
                   
        switch (tokenType) {
		// so here we would need to add all the token types that should not be p_taint
		// this would be any SQL keywords
		  case TK_OR:
		  case TK_AND:
		  case TK_FROM:
		  case TK_LIKE_KW:
		  case TK_TABLE:
		  case TK_DROP:
		  case TK_INSERT:
		  case TK_REPLACE:
		  case TK_LP:
		  case TK_RP:
		  case TK_INTO:
		  case TK_EQ:
		  case TK_UPDATE:
		  case TK_IF:
		  case TK_SELECT:
		  case TK_CONCAT:
		  case TK_UNION:
		  case TK_ALL:
		  case TK_HAVING:
		  case TK_ORDER:
		  case TK_WHERE:
		  case TK_LIMIT:
		  case TK_NOT:
		  case TK_EXISTS:
		  case TK_DELETE:
		  case TK_NULL:
		  case TK_PLUS:
		  case TK_GROUP:
		  case TK_JOIN:
		  case TK_USING:
		  {
		    int taint_detected = 0;
		    for (j = beg; j <= end; ++j)
			{
              if (p_taint[j])
			  {
			      taint_detected = 1;
                  p_taint[j] = APPFW_SECURITY_VIOLATION;
			  }
			}

			if (taint_detected)
			  goto abort_parse;
	      }
		  break;
		}
        break;
      }
    }

	if (end + 2 < strlen(zSql))
	{
	  // detect p_taint comments (assume it's --), this may not be true for various SQL variants
	  if (zSql[end+1] == '-' && zSql[end+2] == '-' &&
	     (p_taint[end+1] || p_taint[end+2]))
	  {
        p_taint[end+1] = APPFW_SECURITY_VIOLATION;
        p_taint[end+2] = APPFW_SECURITY_VIOLATION;
		goto abort_parse;
	  }
	}

  } // end while

  return 1; // this is good

abort_parse:
  return 0;
}

void sqlfw_display_taint(const char *p_msg, const char *p_query, const char *p_taint)
{
  appfw_display_taint(p_msg, p_query, p_taint);
}
