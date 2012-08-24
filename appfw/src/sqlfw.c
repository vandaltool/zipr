//
// Detection of SQL Injections using really simple heuristic:
//   (a) extract strings in binary
//   (b) treat all extracted strings as trusted strings
//   (c) verify SQL query for potential injections by looking for
//       SQL tokens in untrusted portion of query
//
// Reuse sqlite3 parser for parsing query strings
//

#include "appfw.h"
#include "sqlfw.h"

static const char *dbname = "appfw.db";  
static sqlite3 *peasoup_db = NULL;
//extern int fw_numPatterns = 0;
//extern char **fw_sigs;

// read in signature file
// environment variable specifies signature file location
void sqlfw_init()
{
  static int initialized = 0;

  if (initialized) return;

  appfw_init();

  sqlite3_open(dbname, &peasoup_db);

  initialized = 1;
}

// mark parts of string as tainted
static void sqlfw_taint_range(char *taint, char taintValue, int from, int len)
{
  int i;
  for (i = from; i < from+len; ++i)
    taint[i] = taintValue;
}

void sqlfw_establish_taint(const char *query, char *taint)
{
  int i, j, pos;
  int tainted_marking = 1;
  int not_tainted_marking = 0;
  int patternFound;
  char **fw_sigs = appfw_getSignatures();

  if (!fw_sigs)
    return;

  // set taint markings to 'tainted' by default
  sqlfw_taint_range(taint, tainted_marking, 0, strlen(query));

  // use simple linear scan for now
  // list of signature patterns are sorted in reverse length order already
  // unset taint when match is found
  pos = 0;
  while (pos < strlen(query))
  {
//    fprintf(stderr,"starting at position #%d\n", pos);

	// when we'll have reg. expressions for patterns
	// we'd need to make sure we go through all the regex patterns first
	patternFound = 0;
    for (i = 0; i < appfw_getNumSignatures() && !patternFound; ++i)
	{
	  int length_signature = strlen(fw_sigs[i]);
	  if (length_signature >= 1 && length_signature <= (strlen(query) - pos))
	  {
	    if (strncmp(&query[pos], fw_sigs[i], strlen(fw_sigs[i])) == 0)
	    {
		  sqlfw_taint_range(taint, not_tainted_marking, pos, strlen(fw_sigs[i]));
//		  fprintf(stderr,"matched pattern #%d: pos: %d [%s]\n", i, pos, fw_sigs[i]);
	      patternFound = 1;
		}
      }
	}

    pos++;
  }
}

/*
** Run the original sqlite parser on the given SQL string.  
** Look for tainted SQL tokens/keywords
*/
int sqlfw_verify(const char *zSql, char **pzErrMsg){
  Parse *pParse;
  int nErr = 0;                   /* Number of errors encountered */
  int i;                          /* Loop counter */
  void *pEngine;                  /* The LEMON-generated LALR(1) parser */
  int tokenType;                  /* type of the next token */
  int lastTokenParsed = -1;       /* type of the previous token */
  int mxSqlLen = MAX_QUERY_LENGTH;            /* Max length of an SQL string */
  char tainted[MAX_QUERY_LENGTH];
  int j, k;
  int beg, end;

  sqlfw_establish_taint(zSql, tainted);

//  pParse = sqlite3StackAllocZero(db, sizeof(*pParse));
  pParse = sqlite3MallocZero(sizeof(*pParse));

  pParse->db = peasoup_db;

//  fprintf(stderr, "sqlite3_fw(): enter: query string length: %d\n", strlen(zSql));

  // show query
  /*
  fprintf(stderr, "[appfw]: %s\n", zSql);

  fprintf(stderr, "[appfw]: ");

  // show taint
  for (i = 0; i < strlen(zSql); ++i)
    if (tainted[i])
      fprintf(stderr, "X");
	else
      fprintf(stderr, "-");
  fprintf(stderr,"\n");
  */

  pParse->rc = SQLITE_OK;
  pParse->zTail = zSql;
  i = 0;
  pEngine = sqlite3ParserAlloc((void*(*)(size_t))sqlite3Malloc);
  if( pEngine==0 ){
    fprintf(stderr,"Failed to allocated space for pEngine\n");
    return 0;
  }

  while( zSql[i]!=0 ){
    pParse->sLastToken.z = &zSql[i];
    pParse->sLastToken.n = sqlite3GetToken((unsigned char*)&zSql[i],&tokenType);

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
		if (tainted[i])
		{
//          fprintf(stderr,"TAINTED SEMI-COLON DETECTED -- VERY BAD\n");
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

        sqlite3Parser(pEngine, tokenType, pParse->sLastToken, pParse);

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
//   OR is tainted
//   -- is tainted
//   = is tainted
                   
        switch (tokenType) {
		// so here we would need to add all the token types that should not be tainted
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
		    for (j = beg; j <= end; ++j)
			{
              if (tainted[j])
			  {
//		          fprintf(stderr, "TAINTED -- VERY VERY BAD\n");
				  goto abort_parse;
				  break;
			  }
			}
		  break;
		}
        break;
      }
    }

	if (end + 2 < strlen(zSql))
	{
	  if (zSql[end+1] == '-' && zSql[end+2] == '-' &&
	     (tainted[end+1] || tainted[end+2]))
	  {
//        fprintf(stderr, "TAINTED COMMENT -- VERY BAD\n");
		goto abort_parse;
	  }
	}

  } // end while

  return 1; // this is good

abort_parse:
  return 0;
}

