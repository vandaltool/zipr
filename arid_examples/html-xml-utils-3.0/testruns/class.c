/*
 * Routines to check for the occurrence of a class.
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 20 Aug 2000
 * Version: $Id: class.c,v 1.1 2000/08/20 16:31:52 bbos Exp $
 *
 **/

#include "config.h"
#include <assert.h>
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
#include "export.h"
#include "types.e"


/* contains -- check if string contains a certain word, return pointer */
EXPORT const string contains(const string s, const string word)
{
  string t = s;
  unsigned char c;

  while ((t = strstr(t, word))) {
    if ((c = *(t + strlen(word))) && !isspace(c)) t++; /* Not end of word */
    else if (t != s && !isspace(*(t - 1))) t++;	/* Not beginning of word */
    else return t;				/* Found it */
  }
  return NULL;					/* Not found */
}

/* has_class -- check for class=word in list of attributes */
EXPORT Boolean has_class(pairlist attribs, const string word)
{
  pairlist p;

  for (p = attribs; p; p = p->next) {
    if (strcasecmp(p->name, "class") == 0 && contains(p->value, word))
      return True;
  }
  return False;
}

