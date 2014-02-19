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
** Extract out critical tokens
** Return structure of query in p_structure (memory must have been previously allocated)
*/
int sqlfw_verify_s(const char *zSql, char *p_structure) 
{
	int verbose = getenv("APPFW_VERBOSE") ? TRUE : FALSE;
	char *markings = malloc(strlen(zSql)+1);

	if (!peasoupDB)
	{
		if (verbose)
			fprintf(stderr, "peasoupDB is NULL\n");
		return S3_SQL_ERROR;
	}

	// get all the critical keywords
	int result_flag = sqlfw_get_structure(zSql, markings);
	strcpy(p_structure, markings); 

	// are all the critical keywords blessed?
	if (!appfw_establish_taint_fast2(zSql, markings, FALSE))
		result_flag |= S3_SQL_ATTACK_DETECTED;

	if (verbose && ((result_flag | S3_SQL_ATTACK_DETECTED) ||
	                (result_flag | S3_SQL_PARSE_ERROR) ||
	                (result_flag | S3_SQL_ERROR)))
		sqlfw_display_taint("debug", zSql, markings);

	free(markings);
	return result_flag;
}

/*
** Run the original sqlite parser on the given SQL string.  
** Extract out critical tokens
*/
int sqlfw_verify_fast(const char *zSql) 
{
	int verbose = getenv("APPFW_VERBOSE") ? TRUE : FALSE;
	char *tainted = malloc(strlen(zSql)+1);
	if (!peasoupDB)
	{
		if (verbose)
			fprintf(stderr, "peasoupDB is NULL\n");
		return 0;
	}

	// get all the critical keywords
	sqlfw_get_structure(zSql, tainted);
	int success = appfw_establish_taint_fast2(zSql, tainted, FALSE);
	if (!success && verbose)
		sqlfw_display_taint("debug", zSql, tainted);

	free(tainted);
	return success;
}

/*
** Run the original sqlite parser on the given SQL string.  
** Look for tainted SQL tokens/keywords
*/
int sqlfw_verify(const char *zSql, char **pzErrMsg)
{
//  char *tainted = appfw_sqlite3MallocZero(strlen(zSql));

	int verbose = getenv("APPFW_VERBOSE") ? TRUE : FALSE;
	char *tainted = malloc(strlen(zSql)+1);

	if (!peasoupDB)
	{
		if (verbose)
			fprintf(stderr, "peasoupDB is NULL\n");
		return 0;
	}

	int length = strlen(zSql);
	matched_record** matched_signatures = appfw_allocate_matched_signatures(length);

	// figure out taint markings
	// this function is particularly-time consuming -- need to optimize
	sqlfw_establish_taint(zSql, tainted, matched_signatures);

	int success = sqlfw_verify_taint(zSql, tainted, matched_signatures, pzErrMsg);
	
	if (!success && verbose)
		appfw_display_taint("SQL Injection detected", zSql, tainted);

	appfw_deallocate_matched_signatures(matched_signatures, length);

	free(tainted);

	return success;
}

/* Add all the functions you care about here */
//http://dev.mysql.com/doc/refman/5.0/en/information-functions.html
static char *CRITICAL_FUNCTIONS[] = {
	"AES_ENCRYPT", 
	"ANALYSE", 
	"ASCII", 
	"BENCHMARK", 
	"BIT_COUNT", 
	"BIT_LENGTH", 
	"CEIL", 
	"CHAR", 
	"CHARSET", 
	"CHAR_LENGTH", 
	"COLLATION", 
	"CONCAT", 
	"CONVERT",
	"CRC32", 
	"CURRENT_USER", 
	"DATABASE", 
	"DAY", 
	"DAYNAME", 
	"DES_ENCRYPT", 
	"EXEC",
	"FIELD", 
	"FIND_IN_SET", 
	"FLOOR", 
	"FROM_DAYS", 
	"FROM_UNIXTIME", 
	"GROUP_CONCAT",
	"HOUR", 
	"INSTR", 
	"LCASE", 
	"LEFT", 
	"LENGTH", 
	"LOAD_FILE", 
	"LOCATE", 
	"LOWER", 
	"LPAD", 
	"MAX", 
	"MD5", 
	"MID", 
	"MIN", 
	"MINUTE", 
	"MOD", 
	"MONTH", 
	"MONTHNAME",
	"NOW", 
	"OCTET_LENGTH", 
	"ORD", 
	"PI", 
	"POSITION",
	"POW", 
	"QUARTER", 
	"REVERSE", 
	"RIGHT", 
	"ROUND", 
	"RPAD", 
	"SCHEMA", 
	"SESSION_USER", 
	"SHA", 
	"SPACE", 
	"STRCMP", 
	"SUBSTR", 
	"SUBSTRING", 
	"SUBSTRING_INDEX", 
	"SYSTEM_USER", 
	"TRIM", 
	"UCASE", 
	"UNHEX", 
	"UPPER",
	"USER", 
	"USER_NAME", 
	"VERSION", 
	"WEEK", 
	"YEAR",
	NULL
};

/*
** Returns true if the identifier is deemed critical
*/
int is_critical_identifier(const char *identifier)
{
	int i = 0;
	char *fn;
	int len=strlen(identifier);
	/* could have a faster matching algo, but it doesn't matter for now */
	while (fn = (char*) CRITICAL_FUNCTIONS[i++])
	{
		if (len != strlen(fn))
			continue;

		if (strcasecmp(fn, identifier) == 0)
			return 1;
	}

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
  int abortType=0; // 1=TK_ILLEGAL, 2=TK_SEMI
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
      	abortType=1;
      	goto abort_parse;
      }
      case TK_SEMI: {
        pParse->zTail = &zSql[beg];
		if (p_taint[beg] == APPFW_TAINTED)
		{
          p_taint[beg] = APPFW_SECURITY_VIOLATION;
          abortType=2;
		  goto abort_parse;
		}

 		// here we have a SQL terminator; we need to parse the next statement
		// so we recursively call ourself
		if (end+1 < strlen(zSql))
          		sqlfw_verify_taint(&zSql[end+1], &p_taint[end+1], matched_signatures, pzErrMsg);
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
                   
			char temp_identifier[4096];	
        switch (tokenType) {
		// so here we would need to add all the token types that should not be p_taint
		// this would be any SQL keywords
		  case TK_ID: 
		  	strncpy(temp_identifier,&zSql[beg],end - beg + 1);
		  	temp_identifier[end - beg + 1]=0;
			if (!is_critical_identifier(temp_identifier))
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
			  abortType=3;
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
				abortType=4;
			  	goto abort_parse;
			}

	      }
		  break;
	}
        break;
      }
    }

	// handle comments
	if (end + 1 < strlen(zSql))
	{
	  if (zSql[end+1] == '#' && (p_taint[end+1] == APPFW_TAINTED))
	  {
	        p_taint[end+1] = APPFW_SECURITY_VIOLATION;
        abortType=5;
		goto abort_parse;
	  }
	}

	if (end + 2 < strlen(zSql))
	{
		if ((zSql[end+1] == '-' && zSql[end+2] == '-') ||
		    (zSql[end+1] == '/' && zSql[end+2] == '*') ||
	            (zSql[end+1] == '*' && zSql[end+2] == '/'))
	  	{
			if ((p_taint[end+1] == APPFW_TAINTED || p_taint[end+2] == APPFW_TAINTED) || !appfw_is_from_same_signature(matched_signatures, end+1, end+2))
			{
				p_taint[end+1] = APPFW_SECURITY_VIOLATION;
				p_taint[end+2] = APPFW_SECURITY_VIOLATION;
		        abortType=6;
				goto abort_parse;
			}
		}
	}
  } // end while

  appfw_sqlite3ParserFree(pEngine, appfw_sqlite3_free);

  return 1; // this is good

abort_parse:
	if (getenv("APPFW_VERBOSE"))
	{
		fprintf(stderr,"abort_parse parser error: %s\n", pParse->zErrMsg);
		fprintf(stderr,"abort_parse type: %d\n", abortType);
		fprintf(stderr,"abort_parse sqlite err: %s\n", appfw_sqlite3ErrStr(pParse->rc));
		fprintf(stderr,"abort_parse range: [%d]..[%d]: ", beg, end);
		for (k = beg; k <= end; ++k)
			fprintf(stderr,"%c", zSql[k]);
		fprintf(stderr,"\n");
	}

  appfw_sqlite3ParserFree(pEngine, appfw_sqlite3_free);

  return 0;
} // end sqlfw_verify_taint()

// end of line or \0
int look_for_eol(const char *p_str, int p_pos)
{
	int i = 0;

	for (i = p_pos; i < strlen(p_str); ++i)
		if (p_str[i] == '\n')
			return i;

	return i;
}

// end of comment or \0 */
int look_for_eoc(const char *p_str, int p_pos)
{
	int i = 0;

	for (i = p_pos; i < strlen(p_str)-1; ++i)
		if (p_str[i] == '*' && p_str[i+1] == '/')
			return i+1;

	return i+1;
}

char get_violation_marking(char p_current_marking)
{
	if (p_current_marking == APPFW_SECURITY_VIOLATION)
		return APPFW_SECURITY_VIOLATION2;
	else
		return APPFW_SECURITY_VIOLATION;
}

/*
** Run the original sqlite parser on the given SQL string.  
** Identify critical tokens in query
** 
** original code: SQLITE_PRIVATE int sqlite3_sqlite3RunParser(Parse *pParse, const char *zSql, char **pzErrMsg){
*/
int sqlfw_get_structure(const char *zSql, char *p_annot)
{
  Parse *pParse;
  int nErr = 0;                   /* Number of errors encountered */
  int i;                          /* Loop counter */
  void *pEngine;                  /* The LEMON-generated LALR(1) parser */
  int tokenType;                  /* type of the next token */
  int lastTokenParsed = -1;       /* type of the previous token */
  int mxSqlLen = MAX_QUERY_LENGTH;            /* Max length of an SQL string */
  int j, k;
  int beg, end;
  int pos;
  int token_length = 0;
  int comment_1_started = 0;
  int comment_2_started = 0;
  int result_flag = S3_SQL_SAFE;
  int verbose = getenv("APPFW_VERBOSE") ? TRUE : FALSE;

  char mark_violation = APPFW_SECURITY_VIOLATION;

  // terminate recursion if needed
  if (strlen(zSql) <= 0)
    return result_flag;

	// initialized to tainted by default
	appfw_taint_range(p_annot, APPFW_UNKNOWN, 0, strlen(zSql)-1);

  // need to reclaim this space later
  pParse = appfw_sqlite3MallocZero(sizeof(*pParse));

  pParse->db = peasoupDB;

  pParse->rc = SQLITE_OK;
  pParse->zTail = zSql;
  i = 0;
  pEngine = appfw_sqlite3ParserAlloc((void*(*)(size_t))appfw_sqlite3Malloc);
  if( pEngine==0 ){
    fprintf(stderr,"Failed to allocated space for pEngine\n");
    return S3_SQL_PARSE_ERROR;
  }


  while( zSql[i]!=0 ) {

	pParse->sLastToken.z = &zSql[i];
	pParse->sLastToken.n = appfw_sqlite3GetToken((unsigned char*)&zSql[i],&tokenType);

	token_length = pParse->sLastToken.n;
	beg = i;
	i += token_length;
	end = i-1;

    if( i>mxSqlLen ){
      pParse->rc = SQLITE_TOOBIG;
      break;
    }

    switch( tokenType ){
      case TK_SPACE: {
        break;
      }
      case TK_ILLEGAL: {
        result_flag |= S3_SQL_PARSE_ERROR;
	if (verbose)
	{
		fprintf(stderr, "Detected illegal token at pos [%d..%d]\n", beg, end);
		for (i = beg; i <= end; ++i) fprintf(stderr,"%c",zSql[i]);
		fprintf(stderr, "\n");
	}
	continue;
      }
      case TK_SEMI: {
        pParse->zTail = &zSql[beg];
		appfw_taint_range(p_annot, mark_violation, beg, 1);
		mark_violation = get_violation_marking(mark_violation);

 		// here we have a SQL terminator; we need to parse the next statement
		// so we recursively call ourself
		if (end+1 < strlen(zSql))
          return result_flag | sqlfw_get_structure(&zSql[end+1], &p_annot[end+1]);
        else
		{
		  return result_flag; // semicolon was the last character in the entire statement return 
		}
      }
      default: {
	  // show token info
	  
/*
        fprintf(stderr, "\n----------------------\n");
        fprintf(stderr, "token: [");
        for (k = beg; k <= end; ++k)
		  fprintf(stderr,"%c (%d)", zSql[k], p_annot[k]);
		fprintf(stderr, "] type: %d  [%d..%d]\n", tokenType, beg, end);
*/
		
        appfw_sqlite3Parser(pEngine, tokenType, pParse->sLastToken, pParse);

        lastTokenParsed = tokenType;
        if( pParse->rc!=SQLITE_OK ){
                  result_flag |= S3_SQL_PARSE_ERROR;
		  continue;
        }

		char temp_identifier[4096];	
        switch (tokenType) {
		// so here we would need to add all the token types that should not be p_annot
		// this would be any SQL keywords
		  case TK_ID: 
		  	strncpy(temp_identifier,&zSql[beg],end - beg + 1);
		  	temp_identifier[end - beg + 1]=0;
			if (!is_critical_identifier(temp_identifier))
			{
				break;
			}
			// if it's one of the identifier we care about, then fallthrough

		// list is not exhaustive, need to track all relevant ones

		  case TK_EXPLAIN:
		  case TK_ANALYZE:
		  case TK_OR:
		  case TK_AND:
		  case TK_IS:
		  case TK_BETWEEN:
		  case TK_IN:
		  case TK_ISNULL:
		  case TK_NOTNULL:
                  case TK_NE:
                  case TK_EQ:
                  case TK_GT:
                  case TK_LE:
                  case TK_LT:
                  case TK_GE:
		  case TK_FROM:
		  case TK_LIKE_KW:
		  case TK_TABLE:
		  case TK_ALTER:
		  case TK_DROP:
		  case TK_INSERT:
		  case TK_REPLACE:
		  case TK_LP:
		  case TK_RP:
		  case TK_INTO:
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
		  case TK_SET:
		  case TK_ASC:
		  case TK_DESC:
		  case TK_CASE:
		  case TK_WHEN:
		  case TK_THEN:
		  case TK_ELSE:
		  case TK_INDEX:
		  case TK_ROW:
		  case TK_COLUMN:
		  case TK_JOIN_KW:
		  {
			appfw_taint_range(p_annot, mark_violation, beg, token_length);
			mark_violation = get_violation_marking(mark_violation);
	      }
		  break;
	}
        break;
      }
    }

	// handle comments
	if (end + 1 < strlen(zSql))
	{
	  if (zSql[end+1] == '#')
	  {
		pos = look_for_eol(zSql, end+1);
		if (pos >= 0)
			appfw_taint_range_by_pos(p_annot, mark_violation, end+1, pos);
		else
			appfw_taint_range(p_annot, mark_violation, end+1, 1);
		mark_violation = get_violation_marking(mark_violation);
	  }
	}

	if (end + 2 < strlen(zSql))
	{
		if ((zSql[end+1] == '-' && zSql[end+2] == '-') ||
		    (zSql[end+1] == '/' && zSql[end+2] == '*') ||
	            (zSql[end+1] == '*' && zSql[end+2] == '/'))
	  	{
			if (zSql[end+1] == '-')
			{
				pos = look_for_eol(zSql, end+2);
				appfw_taint_range_by_pos(p_annot, mark_violation, end+1, pos);
			}
			else
			{
				pos = look_for_eoc(zSql, end+2);
				appfw_taint_range_by_pos(p_annot, mark_violation, end+1, pos);
			}
			mark_violation = get_violation_marking(mark_violation);
		}
	}
  } // end while

  p_annot[strlen(zSql)-1] = '\0';

  appfw_sqlite3ParserFree(pEngine, appfw_sqlite3_free);

  return result_flag;
}

inline int sqlfw_is_safe(int result_flag)
{
	return result_flag == S3_SQL_SAFE;
}

inline int sqlfw_is_error(int result_flag)
{
	return !sqlfw_is_safe(result_flag);
}

inline int sqlfw_is_attack(int result_flag)
{
	return result_flag & S3_SQL_ATTACK_DETECTED;
}

inline int sqlfw_is_parse_error(int result_flag)
{
	return result_flag & S3_SQL_PARSE_ERROR;
}

