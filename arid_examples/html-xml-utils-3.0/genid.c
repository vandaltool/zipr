/*
 * Generate unique IDs.
 *
 * Copyright © 2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 4 August 2000
 * Version: $Id: genid.c,v 1.5 2003/04/09 10:07:12 bbos Exp $
 **/

#include <config.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_ERRNO_H
#  include <errno.h>
#endif
#include <ctype.h>

#ifdef HAVE_ERRNO_H
#  include <errno.h>
#endif
#ifdef HAVE_SEARCH_H
#  include <search.h>
#else
#  include "search-freebsd.h"
#endif

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
#include <export.h>
#include "heap.e"
#include "types.e"
#include "tree.e"
#include "errexit.e"


#define MAXIDLEN 45				/* Max len of a generated ID */
#define MINIDLEN 5				/* "the" or "a" isn't enough */


static void *idtree = NULL;			/* Sorted tree of IDs */


/* storeID -- remember the existence of an ID (allocates a copy of the ID) */
EXPORT void storeID(string id)
{
  (void) tsearch(newstring(id), &idtree, strcmp);
}


/* gen_id_r -- find some text suitable for an ID recursively */
static void gen_id_r(Tree t, string s, int *len, int minlen, int maxlen)
{
  int i;
  Tree h;

  assert(s);					/* s at least maxlen long */

  /* Loop over children looking for useful text */
  for (h = t->children; h && *len < maxlen - 1; h = h->sister) {
    switch (h->tp) {
      case Text:
	assert(minlen < maxlen);
	if (*len >= minlen) break;
	/* Copy at least minlen characters, if avail, incl. spaces (as '-') */
	for (i = 0; *len < minlen && h->text[i]; i++)
	  if (isalpha(h->text[i])) s[(*len)++] = tolower(h->text[i]);
	  else if (*len == 0) ;			/* Wait for a letter first */
	  else if (h->text[i]=='-' || h->text[i]=='.') s[(*len)++]=h->text[i];
	  else if (isdigit(h->text[i])) s[(*len)++] = h->text[i];
	  else if (isspace(h->text[i]) && s[*len-1] != '-') s[(*len)++]='-';
	/* Then copy up to next space or up to maxlen */
	for (; *len < maxlen - 1 && h->text[i] && !isspace(h->text[i]); i++)
	  if (isalpha(h->text[i])) s[(*len)++] = tolower(h->text[i]);
	  else if (h->text[i]=='-' || h->text[i]=='.') s[(*len)++]=h->text[i];
	  else if (isdigit(h->text[i])) s[(*len)++] = h->text[i];
	  else if (isspace(h->text[i]) && s[*len-1] != '-') s[(*len)++]='-';
	break;
      case Element:				/* Recursive */
	if (*len < minlen) gen_id_r(h, s, len, minlen, maxlen);
	break;
      default:
	break;
    }
  }
  s[*len] = '\0';
}

/* gen_id -- try some heuristics to generate an ID for element t */
EXPORT string gen_id(Tree t)
{
  string s;
  int len = 0;

  if (! (s = malloc(MAXIDLEN + 1))) errexit("Out of memory\n");

  assert(MAXIDLEN > 4);
  gen_id_r(t, s, &len, MINIDLEN, MAXIDLEN - 4);
  if (len == 0) s[len++] = 'x';		/* At least one character */
  if (tfind(s, &idtree, strcmp)) {
    /* No suitable text found or text is already used elsewhere */
    int seqno = 0;
    do {					/* Try adding digits */
      sprintf(s + len, "%d", seqno);
      seqno++;
    } while (seqno != 10000 && tfind(s, &idtree, strcmp));
    if (seqno == 10000) {			/* 10000 tried, giving up... */
      free(s);
      return NULL;
    }
  }
  (void) tsearch(s, &idtree, strcmp);		/* Store it */
  return s;
}

