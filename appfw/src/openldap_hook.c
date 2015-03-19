#ifndef SOLARIS 

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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>

#include <ldap.h>

#include "appfw_ldap.h"

//
// intercepted ldap functions:
//   ldap_search_ext() 
//   ldap_search_ext_s() 
//   ldap_search() 
//   ldap_search_s() 
//   ldap_search_st() 
//

// use a bogus malformed ldap filter to force an error
#define ERROR_VIRTUALIZE_FILTER ")securityViolation("

int
ldap_search_ext(LDAP *ld, LDAP_CONST char *base, int scope, LDAP_CONST char *filter, char **attrs, int attrsonly, LDAPControl **sctrls, LDAPControl **cctrls, struct timeval *timeout, int sizelimit, int *msgidp )
{
  static int (*my_ldap_search_ext)(LDAP *, LDAP_CONST char *, int, LDAP_CONST char *, char **, int, LDAPControl **, LDAPControl **, struct timeval *, int, int *) = NULL;
  if (!my_ldap_search_ext)
  {
    my_ldap_search_ext = dlsym(RTLD_NEXT, "ldap_search_ext");
    appfw_ldap_init(); 
  }

  if (appfw_ldap_verify(filter))
  {
    return my_ldap_search_ext(ld, base, scope, filter, attrs, attrsonly, sctrls, cctrls, timeout, sizelimit, msgidp);
  }
  else
  {
    return my_ldap_search_ext(ld, base, scope, ERROR_VIRTUALIZE_FILTER, attrs, attrsonly, sctrls, cctrls, timeout, sizelimit, msgidp);
  }
}

int
ldap_search_ext_s(LDAP *ld, LDAP_CONST char *base, int scope, LDAP_CONST char *filter, char **attrs, int attrsonly, LDAPControl **sctrls, LDAPControl **cctrls, struct timeval *timeout, int sizelimit, LDAPMessage **res )
{
  static int (*my_ldap_search_ext_s) (LDAP *, LDAP_CONST char *, int, LDAP_CONST char *, char **, int, LDAPControl **, LDAPControl **, struct timeval *, int, LDAPMessage **) = NULL;
  if (!my_ldap_search_ext_s)
  {
    my_ldap_search_ext_s = dlsym(RTLD_NEXT, "ldap_search_ext_s");
    appfw_ldap_init(); 
  }

  if (appfw_ldap_verify(filter))
  {
    return my_ldap_search_ext_s(ld, base, scope, filter, attrs, attrsonly, sctrls, cctrls, timeout, sizelimit, res);
  }
  else
  {
    return my_ldap_search_ext_s(ld, base, scope, ERROR_VIRTUALIZE_FILTER, attrs, attrsonly, sctrls, cctrls, timeout, sizelimit, res);
  }
}

int
ldap_search(LDAP *ld, LDAP_CONST char *base, int scope, LDAP_CONST char *filter, char **attrs, int attrsonly)
{
  static int (*my_ldap_search) (LDAP *, LDAP_CONST char *, int, LDAP_CONST char *, char **, int) = NULL;
  if (!my_ldap_search)
  {
    my_ldap_search = dlsym(RTLD_NEXT, "ldap_search");
    appfw_ldap_init(); 
  }

  if (appfw_ldap_verify(filter))
  {
    return my_ldap_search(ld, base, scope, filter, attrs, attrsonly);
  }
  else
  {
    return my_ldap_search(ld, base, scope, ERROR_VIRTUALIZE_FILTER, attrs, attrsonly);
  }
}

int
ldap_search_s(LDAP *ld, LDAP_CONST char *base, int scope, LDAP_CONST char *filter, char **attrs, int attrsonly, LDAPMessage **res)
{
  static int (*my_ldap_search_s) (LDAP *, LDAP_CONST char *, int, LDAP_CONST char *, char **, int, LDAPMessage **) = NULL;
  if (!my_ldap_search_s)
  {
    my_ldap_search_s = dlsym(RTLD_NEXT, "ldap_search_s");
    appfw_ldap_init(); 
  }

  if (appfw_ldap_verify(filter))
  {
    return my_ldap_search_s(ld, base, scope, filter, attrs, attrsonly, res);
  }
  else
  {
    return my_ldap_search_s(ld, base, scope, ERROR_VIRTUALIZE_FILTER, attrs, attrsonly, res);
  }
}

int
ldap_stearch_st(LDAP *ld, LDAP_CONST char *base, int scope, LDAP_CONST char *filter, char **attrs, int attrsonly, struct timeval *timeout, LDAPMessage **res)
{
  static int (*my_ldap_stearch_st) (LDAP *, LDAP_CONST char *, int, LDAP_CONST char *, char **, int, struct timeval*, LDAPMessage **) = NULL;
  if (!my_ldap_stearch_st)
  {
    my_ldap_stearch_st = dlsym(RTLD_NEXT, "ldap_stearch_st");
    appfw_ldap_init(); 
  }

  if (appfw_ldap_verify(filter))
  {
    return my_ldap_stearch_st(ld, base, scope, filter, attrs, attrsonly, timeout, res);
  }
  else
  {
    return my_ldap_stearch_st(ld, base, scope, ERROR_VIRTUALIZE_FILTER, attrs, attrsonly, timeout, res);
  }
}

#endif
