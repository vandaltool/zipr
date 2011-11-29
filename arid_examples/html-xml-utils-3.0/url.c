/*
 * Routines and data structures to parse URLs
 *
 * Assumes the strings are encoded in UTF-8
 *
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 7 March 1999
 * Version: $Id: url.c,v 1.11 2000/04/01 12:10:08 bbos Exp $
 */
#include <config.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#if STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
# ifndef HAVE_STRSTR
#  include "strstr.e"
# endif
#endif
#include <ctype.h>
#include <regex.h>
#include "export.h"
#include "heap.e"
#include "types.e"

EXPORT typedef struct {
  string full;					/* Full URL as a string */
  string proto;					/* Protocol */
  string user;					/* User name */
  string password;				/* User's password */
  string machine;				/* Domain name or IP number */
  string port;					/* Port number or service */
  string path;					/* Path part of URL */
  string fragment;				/* Fragment ID part of URL */
} *URL;


/* down -- convert a string to lowercase, return pointer to arg */
static string down(string s)
{
  string t;
  for (t = s; *t; t++) *t = tolower(*t);
  return s;
}

/* utf8tohex -- convert UTF-8 to %HH hex encoding, allocate on heap */
static string utf8tohex(const string s)
{
  static string hex = "0123456789ABCDEF";
  string h;
  int i, j;

  newarray(h, 3 * strlen(s) + 1);		/* Usually too much */
  for (i = 0, j = 0; s[i]; i++) {
    if (s[i] <= 127) h[j++] = s[i];
    else { h[j++] = '%'; h[j++] = hex[s[i]/16]; h[j++] = hex[s[i]%16]; }
  }
  h[j] = '\0';
  return h;
}

/* URL_dispose -- free the memory used by a URL struct */
EXPORT void URL_dispose(URL url)
{
  if (url) {
    dispose(url->full);
    dispose(url->proto);
    dispose(url->user);
    dispose(url->password);
    dispose(url->machine);
    dispose(url->port);
    dispose(url->path);
    dispose(url->fragment);
    dispose(url);
  }
}

/* URL_new -- create a new URL struct; return NULL if not a valid URL */
EXPORT URL URL_new(const string url)
{
  static const char* pat =
    "^(([A-Za-z0-9+.-]+):)?(//(([^:@]+)(:([^@]+))?@)?([^:/]+)(:([0-9]+))?)?([^#]*)?(#(.*))?|([^#]*)(#(.*))?$";
  /*  12      2 1 3  45      56 7     76  4 8      89 A      A9 3 B      B C D  DC  E     EF G  GF
   * 2 = proto, 5 = user, 7 = passwd, 8 = machine, A = port, B = path, D = fragment
   * E = path in case it is not a well-formed URL, G = fragment if ditto
   */
# define MAXSUB 18
  static regex_t re;
  static int initialized = 0;
  regmatch_t pm[MAXSUB];
  URL result;

  assert(url != NULL);
  
  /* Compile the regexp, only once */
  if (! initialized) {
    assert(regcomp(&re, pat, REG_EXTENDED) == 0); /* Could be memory, though... */
    initialized = 1;
  }

  /* Match the URL against the pattern; return NULL if no match */
  if (regexec(&re, url, MAXSUB, pm, 0) != 0) return NULL;

  /* Store the various parts */
  new(result);
  result->full = utf8tohex(url);
  result->proto = pm[2].rm_so == -1
    ? NULL : down(newnstring(url, pm[2].rm_eo));
  result->user = pm[5].rm_so == -1
    ? NULL : newnstring(url + pm[5].rm_so, pm[5].rm_eo - pm[5].rm_so);
  result->password = pm[7].rm_so == -1
    ? NULL : newnstring(url + pm[7].rm_so, pm[7].rm_eo - pm[7].rm_so);
  result->machine = pm[8].rm_so == -1
    ? NULL : newnstring(url + pm[8].rm_so, pm[8].rm_eo - pm[8].rm_so);
  result->port = pm[10].rm_so == -1
    ? NULL : newnstring(url + pm[10].rm_so, pm[10].rm_eo - pm[10].rm_so);
  result->path = pm[11].rm_so == -1
    ? NULL : newnstring(url + pm[11].rm_so, pm[11].rm_eo - pm[11].rm_so);
  result->fragment = pm[13].rm_so == -1
    ? NULL : newnstring(url + pm[13].rm_so, pm[13].rm_eo - pm[13].rm_so);
  if (pm[14].rm_so != -1) {
    result->path = newnstring(url + pm[14].rm_so, pm[14].rm_eo - pm[14].rm_so);
    result->fragment = pm[16].rm_so == -1
      ? NULL : newnstring(url + pm[16].rm_so, pm[16].rm_eo - pm[16].rm_so);
  }
  return result;
}

/* merge_path -- make path absolute */
static string  merge_path(const string base, const string path)
{
  static regex_t re;
  static int initialized = 0;
  regmatch_t pm[1];
  string p, s;
  size_t len;

  assert(*path != '/');
  if (base && (p = strrchr(base, '/'))) {
    newarray(s, (p - base) + strlen(path) + 2);
    strncpy(s, base, (p - base) + 1);
    strcpy(s + ((p - base) + 1), path);
  } else {
    newarray(s, strlen(path) + 2);
#if 0
    s[0] = '/';
    strcpy(s + 1, path);
#else
    strcpy(s, path);
#endif
  }

  /* Replace all substrings of form "/xxx/../" with "/" */
  if (! initialized) {
    assert(regcomp(&re, "/[^/]+/\\.\\./", REG_EXTENDED) == 0);
    initialized = 1;
  }
  len = strlen(s);
  while (regexec(&re, s, 1, pm, 0) == 0) {
    memmove(s + pm[0].rm_so, s + (pm[0].rm_eo - 1), len - pm[0].rm_eo + 2);
    len -= pm[0].rm_eo - pm[0].rm_so - 1;
  }

  /* Replace all substrings of the form "/./" with "/" */
  /*len = strlen(s);*/
  for (p = s; (p = strstr(p, "/./")); ) {
    memmove(p, p + 2, len - (p - s) - 1);
    len -= 2;
  }

  return s;    
}

/* URL_absolutize -- make a relative URL absolute */
EXPORT URL URL_absolutize(const URL base, const URL url)
{
  URL abs;
  string s;

  new(abs);
  if (url->proto) {				/* Not relative at all */
    abs->proto = newstring(url->proto);
    abs->user = newstring(url->user);
    abs->password = newstring(url->password);
    abs->machine = newstring(url->machine);
    abs->port = newstring(url->port);
    abs->path = newstring(url->path);
  } else {					/* Missing proto */
    abs->proto = newstring(base->proto);	/* Copy proto from base */
    if (url->machine) {				/* Missing machine */
      abs->user = newstring(url->user);
      abs->password = newstring(url->password);
      abs->machine = newstring(url->machine);
      abs->port = newstring(url->port);
      abs->path = newstring(url->path);
    } else {
      abs->user = newstring(base->user);
      abs->password = newstring(base->password);
      abs->machine = newstring(base->machine);
      abs->port = newstring(base->port);
      if (!url->path || *(url->path) == '/') {	/* Absolute path */
	abs->path = newstring(url->path);
      } else {
	abs->path = merge_path(base->path, url->path);
      }
    }
  }
  abs->fragment = newstring(url->fragment);

  newarray(s, (abs->proto ? strlen(abs->proto) + 1 : 0)
	   + (abs->user ? strlen(abs->user) + 1 : 0)
	   + (abs->password ? strlen(abs->password) + 1 : 0)
	   + (abs->machine ? strlen(abs->machine) + 2 : 0)
	   + (abs->port ? strlen(abs->port) + 1 : 0)
	   + (abs->path ? strlen(abs->path) : 0)
	   + (abs->fragment ? strlen(abs->fragment) + 1 : 0)
	   + 1);
  sprintf(s, "%s%s%s%s%s%s%s%s%s%s%s%s%s",
	  abs->proto ? abs->proto : (string) "",
	  abs->proto ? (string) ":" : (string) "",
	  abs->machine ? (string) "//" : (string) "",
	  abs->user ? abs->user : (string) "",
	  abs->password ? (string) ":" : (string) "",
	  abs->password ? abs->password : (string) "",
	  abs->user ? (string) "@" : (string) "",
	  abs->machine ? abs->machine : (string) "",
	  abs->port ? (string) ":" : (string) "",
	  abs->port ? abs->port : (string) "",
	  abs->path ? abs->path : (string) "",
	  abs->fragment ? (string) "#" : (string) "",
	  abs->fragment ? abs->fragment : (string) "");
  abs->full = utf8tohex(s);
  dispose(s);

  return abs;
}

/* URL_s_absolutize -- make a relative URL absolute */
EXPORT string URL_s_absolutize(const string base, const string url)
{
  URL url1 = URL_new(url), base1 = URL_new(base);
  URL abs = URL_absolutize(base1, url1);
  string result = newstring(abs->full);
  URL_dispose(abs);
  URL_dispose(url1);
  URL_dispose(base1);
  return result;
}
