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
#include "sql_structure.h"

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

void sqlfw_display_taint(const char *p_msg, const char *p_query, const char *p_taint)
{
  appfw_display_taint(p_msg, p_query, p_taint);
}

/*
** Run the original sqlite parser on the given SQL string.  
** Extract out critical tokens
** Return structure of query in p_critical (memory must have been previously allocated)
** Used with web S3 prototype
*/
int sqlfw_verify_s(const char *zSql, char *p_critical) 
{
	int verbose = getenv("APPFW_VERBOSE") ? TRUE : FALSE;

	struct timeval blah,  blah2, blah3;
	double elapsed1, elapsed2;

	if (!peasoupDB)
	{
		if (verbose)
			fprintf(stderr, "peasoupDB is NULL\n");
		return S3_SQL_ERROR;
	}

	if(verbose)
	{
		gettimeofday(&blah,NULL);
		fprintf(stdout, "\nsqlfw: start: %d:%d\n", blah.tv_sec, blah.tv_usec);
	}

	int len = strlen(zSql)+1;
	char *markings = malloc(len);
	char *structure = malloc(len);

	bzero(structure, len);

	// get all the critical keywords
	int result_flag = sqlfw_get_structure(zSql, markings, structure);
	strcpy(p_critical, markings); 

	if(verbose)
	{
		gettimeofday(&blah2,NULL);
		elapsed1 = (blah2.tv_sec * 1000000.0 + blah2.tv_usec - (blah.tv_sec * 1000000.0 + blah.tv_usec)) / 1000.0;
		fprintf(stdout, "\nsqlfw: past parse: %d:%d\n", blah2.tv_sec, blah2.tv_usec);
	}

	// was this query structure deemed safe before?
	if (sqlfw_is_safe(result_flag) && findQueryStructure(structure))
	{
		free(structure);
		return S3_SQL_SAFE;
	}

	if (verbose)
		fprintf(stdout, "query structure: %s\n", structure);

	// are all the critical keywords blessed?
	if (!appfw_establish_taint_fast2(zSql, markings, FALSE))
		result_flag |= S3_SQL_ATTACK_DETECTED;
	else
	{
		// cache up the query structure as it is safe
		if (sqlfw_is_safe(result_flag))
		{
			addQueryStructure(structure);
		}
	}

	if(verbose)
	{
		gettimeofday(&blah3,NULL);
		elapsed2 = (blah3.tv_sec * 1000000.0 + blah3.tv_usec - (blah2.tv_sec * 1000000.0 + blah2.tv_usec)) / 1000.0;
		fprintf(stdout, "\nsqlfw: past match: %d:%d\n", blah3.tv_sec, blah3.tv_usec);
		elapsed2 = ((blah3.tv_sec - blah2.tv_sec) * 1000000.0 + (blah3.tv_usec - blah2.tv_usec) ) / 1000.0;
	}


	if (verbose && ((result_flag | S3_SQL_ATTACK_DETECTED) ||
	                (result_flag | S3_SQL_PARSE_ERROR) ||
	                (result_flag | S3_SQL_ERROR)))
		sqlfw_display_taint("debug", zSql, markings);

	free(markings);

	if(verbose)
	{
		gettimeofday(&blah3,NULL);
		elapsed2 = (blah3.tv_sec * 1000000.0 + blah3.tv_usec - (blah2.tv_sec * 1000000.0 + blah2.tv_usec)) / 1000.0;
		elapsed2 = ((blah3.tv_sec - blah2.tv_sec) * 1000000.0 + (blah3.tv_usec - blah2.tv_usec) ) / 1000.0;
	}

	if(verbose)
	{
		fprintf(stdout, "\tsqlfw[parse]: elapsed(msec): %f\n", elapsed1);
		fprintf(stdout, "\tsqlfw[match]: elapsed(msec): %f\n\n", elapsed2);
		fprintf(stdout, "sqlfw: end: %d:%d\n", blah3.tv_sec, blah3.tv_usec);
	}

	return result_flag;
}

/*
** Run the original sqlite parser on the given SQL string.  
** Extract out critical tokens
*/
int sqlfw_verify(const char *zSql, char **errMsg) 
{
	int verbose = getenv("APPFW_VERBOSE") ? TRUE : FALSE;
	int len = strlen(zSql)+1;
	char *tainted = malloc(len);
	char *structure = malloc(len);
	bzero(structure, len);
	if (!peasoupDB)
	{
		if (verbose)
			fprintf(stderr, "peasoupDB is NULL\n");
		return 0;
	}

	// get all the critical keywords
	sqlfw_get_structure(zSql, tainted, structure);
	int success = appfw_establish_taint_fast2(zSql, tainted, FALSE);
	if (!success && verbose)
		appfw_display_taint("SQL Injection detected", zSql, tainted);

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
int sqlfw_get_structure(const char *zSql, char *p_annot, char *p_structure)
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

  // by default mark critial tokens as security violations
  // in a subsequent stage, we will attempt to bless them via dna shotgun sequencing
  char mark_critical_token = APPFW_SECURITY_VIOLATION;

  // terminate recursion if needed
  if (strlen(zSql) <= 0)
    return result_flag;

	// initialized to tainted by default
	appfw_taint_range(p_annot, APPFW_UNKNOWN, 0, strlen(zSql)-1);

  // @todo: need to reclaim this space later
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
		appfw_taint_range(p_annot, mark_critical_token, beg, 1);
		mark_critical_token = get_violation_marking(mark_critical_token);
		strcat(p_structure, "; ");

 		// here we have a SQL terminator; we need to parse the next statement
		// so we recursively call ourself
		if (end+1 < strlen(zSql))
          return result_flag | sqlfw_get_structure(&zSql[end+1], &p_annot[end+1], p_structure);
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
	
		
		  case TK_STRING: 
			strcat(p_structure, "d ");
			break;
			
		  case TK_ID: 
		  	strncpy(temp_identifier,&zSql[beg],end - beg + 1);
		  	temp_identifier[end - beg + 1]=0;

			if (!is_critical_identifier(temp_identifier))
			{
				strcat(p_structure, temp_identifier);
				strcat(p_structure, " ");
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
			appfw_taint_range(p_annot, mark_critical_token, beg, token_length);
			mark_critical_token = get_violation_marking(mark_critical_token);

			strncat(p_structure, &zSql[beg], token_length);
			strcat(p_structure, " ");
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
			appfw_taint_range_by_pos(p_annot, mark_critical_token, end+1, pos);
		else
			appfw_taint_range(p_annot, mark_critical_token, end+1, 1);
		mark_critical_token = get_violation_marking(mark_critical_token);
		strcat(p_structure, "#");
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
				appfw_taint_range_by_pos(p_annot, mark_critical_token, end+1, pos);
				strncat(p_structure, "-- ", 2);
			}
			else
			{
				pos = look_for_eoc(zSql, end+2);
				appfw_taint_range_by_pos(p_annot, mark_critical_token, end+1, pos);
			}
			mark_critical_token = get_violation_marking(mark_critical_token);
			strncat(p_structure, &zSql[end+1], pos-end+1);
		}
	}
  } // end while

  p_annot[strlen(zSql)-1] = '\0';

  appfw_sqlite3ParserFree(pEngine, appfw_sqlite3_free);

  return result_flag;
}

int sqlfw_is_safe(int result_flag)
{
	return result_flag == S3_SQL_SAFE;
}

int sqlfw_is_error(int result_flag)
{
	return !sqlfw_is_safe(result_flag);
}

int sqlfw_is_attack(int result_flag)
{
	return result_flag & S3_SQL_ATTACK_DETECTED;
}

int sqlfw_is_parse_error(int result_flag)
{
	return result_flag & S3_SQL_PARSE_ERROR;
}

void sqlfw_init_from_file(const char *p_file)
{
	sqlfw_init();
	initQueryStructureCache(p_file);
}

void sqlfw_save_query_structure_cache(const char *p_file)
{
	saveQueryStructureCache(p_file);
}
