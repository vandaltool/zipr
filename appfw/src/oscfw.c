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

void osc_parse(char* to_parse, char* taint_markings);


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

  	appfw_empty_taint(p_command, p_taint);

//	if(getenv("APPFW_VERBOSE"))
//	  	appfw_display_taint("Debugging OS Command", p_command, p_taint);

	osc_parse((char*)p_command, (char*)p_taint);
	// post: taint markings will be 'security violation' wherever there's a 
	//       critical keyword

	if(getenv("APPFW_VERBOSE"))
  		appfw_display_taint("Debug OSC after parse", p_command, p_taint);

  	int OK=appfw_establish_taint_fast2(p_command, p_taint, TRUE);

	if(getenv("APPFW_VERBOSE"))
		if (OK)
			fprintf(stderr,"verify OK\n");
		else
			fprintf(stderr,"verify NOT okay\n");

	return OK;
}


// insert function below to parse & verify taint
int oscfw_verify(const char *p_command, char *p_taint)
{
	return oscfw_verify_fast(p_command, p_taint);
}
