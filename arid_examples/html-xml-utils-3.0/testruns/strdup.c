/*
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 31 Mar 2000
 * Version: $Id: strdup.c,v 1.1 2000/03/31 09:28:49 bbos Exp $
 **/
#include <config.h>
#include <stdlib.h>
#include "export.h"

#ifndef HAVE_STRDUP
/* strdup -- allocate a copy of a string on the heap; NULL if no memory */
EXPORT char *strdup(const char *s)
{
  char *t;

  if ((t = malloc((strlen(s) + 1) * sizeof(*s)))) strcpy(t, s);
  return t;
}
#endif /* HAVE_STRDUP */
