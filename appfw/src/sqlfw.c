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
static const char *DEFAULT_APPFW_FILE = "appfw.db";
static appfw_sqlite3 *peasoupDB = NULL;
static int sqlfw_initialized = 0;

// read in signature file
// environment variable specifies signature file location
void sqlfw_init()
{
	if (sqlfw_isInitialized()) return;

 
	if (getenv("APPFW_VERBOSE"))
		fprintf(stderr, "appfw::sqlfw_init(): called\n");
	appfw_init();

	if (appfw_isInitialized())
	{
		if (getenv(dbPathEnv))
		{
			if (appfw_sqlite3_open(getenv(dbPathEnv), &peasoupDB) == SQLITE_OK)
				sqlfw_initialized = 1;
		}	
		else
		{
			if (appfw_sqlite3_open(DEFAULT_APPFW_FILE, &peasoupDB) == SQLITE_OK)
				sqlfw_initialized = 1;
		}
	}
}

// returns whether initialized
int sqlfw_isInitialized()
{
  return sqlfw_initialized;
}

void sqlfw_establish_taint(const char *query, char *taint, matched_record** matched_signatures)
{
  char **fw_sigs = appfw_getSignatures();

  if (!fw_sigs || !sqlfw_isInitialized())
    return;

  appfw_establish_taint(query, taint, matched_signatures,FALSE);
}

void sqlfw_display_taint(const char *p_msg, const char *p_query, const char *p_taint)
{
  appfw_display_taint(p_msg, p_query, p_taint);
}

/*
** Run the original sqlite parser on the given SQL string.  
** Look for tainted SQL tokens/keywords
*/
int sqlfw_verify(const char *zSql, char **pzErrMsg){
//  char *tainted = appfw_sqlite3MallocZero(strlen(zSql));

	char *tainted = malloc(strlen(zSql)+1);

	if (!peasoupDB)
	{
		if (getenv("APPFW_VERBOSE"))
			fprintf(stderr, "peasoupDB is NULL\n");
		return 0;
	}

	int length = strlen(zSql);
	matched_record** matched_signatures = appfw_allocate_matched_signatures(length);

	// figure out taint markings
	// this function is particularly-time consuming -- need to optimize
	sqlfw_establish_taint(zSql, tainted, matched_signatures);

	int success = sqlfw_verify_taint(zSql, tainted, matched_signatures, pzErrMsg);
	if (!success)
		appfw_display_taint("SQL Injection detected", zSql, tainted);

	appfw_deallocate_matched_signatures(matched_signatures, length);

	free(tainted);

	return success;
}

/*
**
**
*/
int is_critical_identifier(const char *identifier, int len)
{
	int i;

	if (strncasecmp("CHAR", identifier, len) == 0 ||
	    strncasecmp("MD5", identifier, len) == 0 ||
	    strncasecmp("USER", identifier, len) == 0 ||
	    strncasecmp("COLLATION", identifier, len) == 0 ||
	    strncasecmp("UNHEX", identifier, len) == 0 ||
	    strncasecmp("ASCII", identifier, len) == 0 ||
	    strncasecmp("ORD", identifier, len) == 0 ||
	    strncasecmp("DAYNAME", identifier, len) == 0 ||
	    strncasecmp("MONTHNAME", identifier, len) == 0 ||
	    strncasecmp("AES_ENCRYPT", identifier, len) == 0 ||
	    strncasecmp("DES_ENCRYPT", identifier, len) == 0 ||
	    strncasecmp("CEIL", identifier, len) == 0 ||
	    strncasecmp("FLOOR", identifier, len) == 0 ||
	    strncasecmp("PI", identifier, len) == 0 ||
	    strncasecmp("POW", identifier, len) == 0 ||
	    strncasecmp("VERSION", identifier, len) == 0 ||
	    strncasecmp("CONCAT", identifier, len) == 0 ||
	    strncasecmp("NOW", identifier, len) == 0 ||
	    strncasecmp("DAY", identifier, len) == 0 ||
	    strncasecmp("WEEK", identifier, len) == 0 ||
	    strncasecmp("MONTH", identifier, len) == 0 ||
	    strncasecmp("YEAR", identifier, len) == 0 ||
	    strncasecmp("QUARTER", identifier, len) == 0 ||
	    strncasecmp("CRC32", identifier, len) == 0 ||
	    strncasecmp("SUBSTR", identifier, len) == 0 ||
	    strncasecmp("SUBSTRING", identifier, len) == 0 ||
	    strncasecmp("MID", identifier, len) == 0 ||
	    strncasecmp("LPAD", identifier, len) == 0 ||
	    strncasecmp("RPAD", identifier, len) == 0 ||
	    strncasecmp("LEFT", identifier, len) == 0 ||
	    strncasecmp("REVERSE", identifier, len) == 0 ||
	    strncasecmp("SPACE", identifier, len) == 0 ||
	    strncasecmp("TRIM", identifier, len) == 0 ||
	    strncasecmp("LOCATE", identifier, len) == 0 ||
	    strncasecmp("POSITION", identifier, len) == 0 ||
	    strncasecmp("FIND_IN_SET", identifier, len) == 0 ||
	    strncasecmp("STRCMP", identifier, len) == 0 ||
	    strncasecmp("MOD", identifier, len) == 0 ||
	    strncasecmp("FIELD", identifier, len) == 0 ||
	    strncasecmp("UCASE", identifier, len) == 0 ||
	    strncasecmp("LCASE", identifier, len) == 0 ||
	    strncasecmp("LOWER", identifier, len) == 0 ||
	    strncasecmp("UPPER", identifier, len) == 0 ||
	    strncasecmp("SHA", identifier, len) == 0 ||
	    strncasecmp("MIN", identifier, len) == 0 ||
	    strncasecmp("MAX", identifier, len) == 0 ||
	    strncasecmp("LOAD_FILE", identifier, len) == 0 ||
	    strncasecmp("LENGTH", identifier, len) == 0 ||
	    strncasecmp("BIT_LENGTH", identifier, len) == 0 ||
	    strncasecmp("CHAR_LENGTH", identifier, len) == 0 ||
	    strncasecmp("OCTET_LENGTH", identifier, len) == 0 ||
	    strncasecmp("BIT_COUNT", identifier, len) == 0 ||
	    strncasecmp("BENCHMARK", identifier, len) == 0)
	{
//		fprintf(stderr,"Critical identifier found: [%d] %c%c\n", len, identifier[0], identifier[1]);
		return 1;
	}
	else
		return 0;
}

/*
** Run the original sqlite parser on the given SQL string.  
** Look for tainted SQL tokens/keywords
** Sets the taint array
** 
** original code: SQLITE_PRIVATE int sqlite3_sqlite3RunParser(Parse *pParse, const char *zSql, char **pzErrMsg){
*/
int sqlfw_verify_taint(const char *zSql, char *p_taint, matched_record** matched_signatures, char **pzErrMsg){
  Parse *pParse;
  int nErr = 0;                   /* Number of errors encountered */
  int i;                          /* Loop counter */
  void *pEngine;                  /* The LEMON-generated LALR(1) parser */
  int tokenType;                  /* type of the next token */
  int lastTokenParsed = -1;       /* type of the previous token */
  int mxSqlLen = MAX_QUERY_LENGTH;            /* Max length of an SQL string */
  int j, k;
  int beg, end;

  // terminate recursion if needed
  if (strlen(zSql) <= 0)
    return 1;

  if (getenv("APPFW_VERBOSE"))
    sqlfw_display_taint("debug", zSql, p_taint);

  // need to reclaim this space later
  pParse = appfw_sqlite3MallocZero(sizeof(*pParse));

  pParse->db = peasoupDB;

  pParse->rc = SQLITE_OK;
  pParse->zTail = zSql;
  i = 0;
  pEngine = appfw_sqlite3ParserAlloc((void*(*)(size_t))appfw_sqlite3Malloc);
  if( pEngine==0 ){
    fprintf(stderr,"Failed to allocated space for pEngine\n");
    return 0;
  }

  while( zSql[i]!=0 ) {
    pParse->sLastToken.z = &zSql[i];
    pParse->sLastToken.n = appfw_sqlite3GetToken((unsigned char*)&zSql[i],&tokenType);

	beg = i;
    i += pParse->sLastToken.n;
	end = i-1;

    if( i>mxSqlLen ){
      pParse->rc = SQLITE_TOOBIG;
      break;
    }

//	fprintf(stderr, "token: [ ] type: %d [%d..%d]\n", tokenType, beg, end);

    switch( tokenType ){
      case TK_SPACE: {
        break;
      }
      case TK_ILLEGAL: {
        goto abort_parse;
      }
      case TK_SEMI: {
        pParse->zTail = &zSql[beg];
		if (p_taint[beg] == APPFW_TAINTED)
		{
          p_taint[beg] = APPFW_SECURITY_VIOLATION;
		  goto abort_parse;
		}

 		// here we have a SQL terminator; we need to parse the next statement
		// so we recursively call ourself
		if (end+1 < strlen(zSql))
          return sqlfw_verify_taint(&zSql[end+1], &p_taint[end+1], matched_signatures, pzErrMsg);
        else
		{
		  return 1; // semicolon was the last character in the entire statement return success
		}
      }
      default: {
	  // show token info
	  
/*
        fprintf(stderr, "\n----------------------\n");
        fprintf(stderr, "token: [");
        for (k = beg; k <= end; ++k)
		  fprintf(stderr,"%c (%d)", zSql[k], p_taint[k]);
		fprintf(stderr, "] type: %d  [%d..%d]\n", tokenType, beg, end);
*/
		
        appfw_sqlite3Parser(pEngine, tokenType, pParse->sLastToken, pParse);

        lastTokenParsed = tokenType;
        if( pParse->rc!=SQLITE_OK ){
		  continue;

		  /*
    		fprintf(stderr,"fwsql_verify(): Error in parsing: \n"); 
          goto abort_parse;
		  */
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
		  case TK_ID: 
			if (!is_critical_identifier(&zSql[beg], end - beg + 1))
			{
				break;
			}
			// if it's one of the identifier we care about, then fallthrough
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
              if (p_taint[j] == APPFW_TAINTED)
			  {
			      taint_detected = 1;
                  p_taint[j] = APPFW_SECURITY_VIOLATION;
			  }
			}

			if (taint_detected)
			{
			  goto abort_parse;
			}
			else if (!appfw_is_from_same_signature(matched_signatures, beg, end))
			{
  				if (getenv("APPFW_VERBOSE"))
					fprintf(stderr,"not same signature at pos [%d..%d]\n", beg, end);

        		for (k = beg; k <= end; ++k)
				{
//					fprintf(stderr,"%c", zSql[k]);
					p_taint[k] = APPFW_SECURITY_VIOLATION;
				}
//				fprintf(stderr,"%n");
			  	goto abort_parse;
			}

	      }
		  break;
	}
        break;
      }
    }

	if (end + 1 < strlen(zSql))
	{
	  if (zSql[end+1] == '#' && (p_taint[end+1] == APPFW_TAINTED))
	  {
	        p_taint[end+1] = APPFW_SECURITY_VIOLATION;
		goto abort_parse;
	  }
	}

	if (end + 2 < strlen(zSql))
	{
	  // detect p_taint comments (assume it's --), this may not be true for various SQL variants
	  if (zSql[end+1] == '-' && zSql[end+2] == '-' &&
	     (p_taint[end+1] == APPFW_TAINTED || p_taint[end+2] == APPFW_TAINTED))
	  {
        p_taint[end+1] = APPFW_SECURITY_VIOLATION;
        p_taint[end+2] = APPFW_SECURITY_VIOLATION;
		goto abort_parse;
	  }

	  if (((zSql[end+1] == '/' && zSql[end+2] == '*') ||
	       (zSql[end+1] == '*' && zSql[end+2] == '/')) &&
	     (p_taint[end+1] == APPFW_TAINTED || p_taint[end+2] == APPFW_TAINTED))
	  {
        p_taint[end+1] = APPFW_SECURITY_VIOLATION;
        p_taint[end+2] = APPFW_SECURITY_VIOLATION;
		goto abort_parse;
	  }
        
	}
  } // end while

  appfw_sqlite3ParserFree(pEngine, appfw_sqlite3_free);

  return 1; // this is good

abort_parse:
	if (getenv("APPFW_VERBOSE"))
	{
		fprintf(stderr,"abort_parse: %s\n", pParse->zErrMsg);
		fprintf(stderr,"abort_parse: %s\n", appfw_sqlite3ErrStr(pParse->rc));
		}
  appfw_sqlite3ParserFree(pEngine, appfw_sqlite3_free);
  return 0;
}

