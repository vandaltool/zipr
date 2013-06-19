#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "appfw.h"
#include "ldap.h"

void xq_parse(char* to_parse, char* taint_markings);

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
    matched_record** matched_signatures = appfw_allocate_matched_signatures(length);

	char *p_taint=malloc(length+1);
  	appfw_establish_taint(p_filter, p_taint, matched_signatures);

	appfw_deallocate_matched_signatures(matched_signatures, length);

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
