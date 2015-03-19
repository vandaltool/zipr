/*
 * Copyright (c) 2013, 2014 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

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
	int i;
	int length = strlen(p_command);

  	appfw_empty_taint(p_command, p_taint);

	if(getenv("APPFW_VERBOSE"))
	  	appfw_display_taint("Debugging OS Command", p_command, p_taint);

	osc_parse((char*)p_command, (char*)p_taint);
	// post: taint markings will be 'security violation' wherever there's a 
	//       critical keyword


  	int OK = appfw_establish_taint_fast2(p_command, p_taint, TRUE, FALSE);

	// heuristic -- if first critical token is tainted (a security violation)
	//              then allow the command through
	for (i = 0; i < length; ++i)
	{
		if (is_blessed(p_taint[i]))
			break;

		if (is_security_violation(p_taint[i]) )
		{
			appfw_log("OSC: first critical token is tainted, but bless anyways");
			OK = 1;
			break;
		}
	}

	if (OK)
	{
		appfw_log("OSC: command verify OK");
	}
	else
	{
		appfw_log_taint("OS Command Injection detected", p_command, p_taint);
	}

	return OK;
}


// insert function below to parse & verify taint
int oscfw_verify(const char *p_command, char *p_taint)
{
	return oscfw_verify_fast(p_command, p_taint);
}
