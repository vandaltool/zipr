
#include "appfw.h"
#include "xqfw.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

void xq_parse(char* to_parse, char* taint_markings);


static int xqfw_initialized = 0;

// read in signature file
// environment variable specifies signature file location
void xqfw_init()
{
  if (xqfw_isInitialized()) return;

  appfw_init();

  xqfw_initialized = 1;
}

// returns whether initialized
int xqfw_isInitialized()
{
  return xqfw_initialized;
}

// insert function below to parse & verify taint
int xqfw_verify(const char *p_command)
{

	char *p_taint=malloc(strlen(p_command)+1);
  	appfw_establish_taint(p_command, p_taint);
	if(getenv("APPFW_VERBOSE"))
	  	appfw_display_taint("Debugging OS Command", p_command, p_taint);

	xq_parse((char*)p_command, (char*)p_taint);

	if(getenv("APPFW_VERBOSE"))
  		appfw_display_taint("Debug XQ after parse", p_command, p_taint);

  	// return code is really a boolean
  	// return > 0 if success
  	// return 0 if failure
	int i;
	for(i=0;i<strlen(p_command);i++)
	{
		if(getenv("APPFW_PRINTCOMMAND_VERBOSE"))
			fprintf(stderr, "Verifyig p_command[%d]=%d\n", i, p_command[i]);
		if(p_taint[i]==APPFW_SECURITY_VIOLATION)
		{
			if(getenv("APPFW_VERBOSE"))
				fprintf(stderr,"verify NOT okayK\n");
			return 0;
		}
	}
	if(getenv("APPFW_VERBOSE"))
		fprintf(stderr,"verify OK\n");
	return 1;
}
