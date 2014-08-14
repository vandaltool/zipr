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
#include "oscfw.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>

void osc_parse(char* to_parse, char* taint_markings,matched_record** matched_signatures);


static int oscfw_initialized = 0;

// read in signature file
// environment variable specifies signature file location
void oscfw_init()
{
  if (oscfw_isInitialized()) return;

  appfw_init();

  oscfw_initialized = 1;
}

// returns whether initialized
int oscfw_isInitialized()
{
  return oscfw_initialized;
}


int oscfw_verify_fast(const char *p_command, char *p_taint)
{
	int length = strlen(p_command);

  	appfw_empty_taint(p_command, p_taint, NULL, TRUE);

//	if(getenv("APPFW_VERBOSE"))
//	  	appfw_display_taint("Debugging OS Command", p_command, p_taint);

	osc_parse((char*)p_command, (char*)p_taint, NULL);
	// post: taint markings will be 'security violation' wherever there's a 
	//       critical keyword

	if(getenv("APPFW_VERBOSE"))
  		appfw_display_taint("Debug OSC after parse", p_command, p_taint);

  	int OK=appfw_establish_taint_fast(p_command, p_taint, TRUE);

	if(getenv("APPFW_VERBOSE"))
		if (OK)
			fprintf(stderr,"verify OK\n");
		else
			fprintf(stderr,"verify NOT okay\n");

	return OK;
}

int oscfw_verify_slow(const char *p_command, char *p_taint)
{
	int length = strlen(p_command);
    	matched_record** matched_signatures = appfw_allocate_matched_signatures(length);

  	appfw_empty_taint(p_command, p_taint, NULL,TRUE);
   	appfw_establish_taint(p_command, p_taint, matched_signatures,TRUE);
	if(getenv("APPFW_VERBOSE"))
	  	appfw_display_taint("Debugging OS Command", p_command, p_taint);

	// pre: taint markings = tainted or blessed
	osc_parse((char*)p_command, (char*)p_taint, NULL);
	// post: if any critical keywords are tainted, then taint markings = security violation

	if(getenv("APPFW_VERBOSE"))
  		appfw_display_taint("Debug OSC after parse", p_command, p_taint);

	appfw_deallocate_matched_signatures(matched_signatures, length);

  	// return code is really a boolean
  	// return > 0 if success
  	// return 0 if failure
	int i;
	int OK=TRUE;

	// first keyword is marked as BLESSED_KEYWORD or SECURITY_VIOLATION:
	//             cat README
	//         ttttkkkttttttt
	//         ttttvvvttttttt

	int first_keyword = 1;
	int first_keyword_seen = 1;
	for(i=0;i<strlen(p_command);i++)
	{
		if(getenv("APPFW_PRINTCOMMAND_VERBOSE"))
			fprintf(stderr, "Verifying p_command[%d]=%d\n", i, p_command[i]);

		if (p_taint[i] == APPFW_BLESSED_KEYWORD ||
		    p_taint[i] == APPFW_SECURITY_VIOLATION ||
		    p_taint[i] == APPFW_SECURITY_VIOLATION2)
		{
			first_keyword_seen = 1;
		}

		if (first_keyword_seen &&
			(p_taint[i] != APPFW_BLESSED_KEYWORD ||
			 p_taint[i] != APPFW_SECURITY_VIOLATION ||
			 p_taint[i] != APPFW_SECURITY_VIOLATION2))
		{
			first_keyword = 0;
		}
		
		// factor this out
                //     echo foobar; ls -lt
		//     kkkktttttttvtvvtbbb
		if(p_taint[i]==APPFW_SECURITY_VIOLATION || p_taint[i]==APPFW_SECURITY_VIOLATION2)
		{
			// 20140815 new policy: if the command starts as a security violation,
			//          assume a false positivte, and treat command as safe
			if (first_keyword)
				return TRUE;

			OK=FALSE;
			break;
		}
	}

	if(getenv("APPFW_VERBOSE"))
	{
		if(OK)
			fprintf(stderr,"verify OK\n");
		else
			fprintf(stderr,"verify NOT okay\n");
	}
	return OK;
}

// insert function below to parse & verify taint
int oscfw_verify(const char *p_command, char *p_taint)
{
	return oscfw_verify_slow(p_command, p_taint);
}
