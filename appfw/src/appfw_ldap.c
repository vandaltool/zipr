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

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "appfw.h"
#include "ldap.h"

static int ldap_initialized = 0;

void appfw_ldap_init()
{
  if (appfw_ldap_isInitialized()) return;

  appfw_init();

  ldap_initialized = 1;
}

// returns whether initialized
int appfw_ldap_isInitialized()
{
  return ldap_initialized;
}

int appfw_ldap_isFilterOperator(char c)
{
	// these are the 10 special characters to look for in a LDAP filter
	return (
  		c == '|' ||
 	 	c == '&' ||
		c == '(' ||
		c == ')' ||
		c == '*' ||
		c == '<' ||
		c == '>' ||
		c == '=' ||
		c == '~' ||
		c == '!');
}

// insert function below to parse & verify taint
// return code is really a boolean
// return > 0 if success
// return 0 if failure
int appfw_ldap_verify(const char *p_filter)
{
	int length = strlen(p_filter);

	char *p_taint=malloc(length+1);
  	appfw_establish_blessed(p_filter, p_taint, FALSE);

	if(getenv("APPFW_VERBOSE"))
	  	appfw_display_taint("Debugging LDAP filter", p_filter, p_taint);

	int i;
	for(i=0;i<strlen(p_filter);i++)
	{
		if(appfw_ldap_isFilterOperator(p_filter[i]) && (p_taint[i]!=APPFW_BLESSED))
		{
			p_taint[i] = APPFW_SECURITY_VIOLATION;

			if(getenv("APPFW_VERBOSE"))
	  			appfw_display_taint("Security violation detected", p_filter, p_taint);

	    		free(p_taint);
			return 0;
		}
	}

	free(p_taint);
	return 1;
}
