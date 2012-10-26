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

// insert function below to parse & verify taint
int oscfw_verify(const char *p_command, char *p_taint)
{
  	appfw_establish_taint(p_command, p_taint);
  	appfw_display_taint("Debugging OS Command", p_command, p_taint);

	osc_parse((char*)p_command, (char*)p_taint);

  	appfw_display_taint("Debug OSC after parse", p_command, p_taint);

  	// return code is really a boolean
  	// return > 0 if success
  	// return 0 if failure
	int i;
	for(i=0;i<strlen(p_command);i++)
	{
		if(p_taint[i]==APPFW_SECURITY_VIOLATION)
			return 0;
	}
	return 1;
}
