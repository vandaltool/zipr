/*
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 31 Mar 2000
 * Version: $Id: strstr.c,v 1.1 2000/03/31 09:28:49 bbos Exp $
 **/
#include <config.h>
#include "export.h"

#ifndef HAVE_STRSTR
EXPORT char *strstr(const char *haystack, const char *needle)
{
  char *s, *t, *u;

  if (! needle) return haystack;		/* No needle */
  for (s = haystack; *s; s++) {
    for (t = needle, u = s; *t == *u && *t; t++, u++);
    if (! *t) return s;				/* Found it */
  }
  return NULL;					/* Not found */
}
#endif /* HAVE_STRSTR */
