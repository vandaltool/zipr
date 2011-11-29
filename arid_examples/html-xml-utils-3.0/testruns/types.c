/*
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 1997
 * Version: $Id: types.c,v 1.13 2003/01/21 19:45:38 bbos Exp $
 **/
#include <config.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef HAVE_STRING_H
#  include <string.h>
#elif HAVE_STRINGS_H
#  include <strings.h>
#endif
#include "export.h"
#include "heap.e"

EXPORT typedef enum { False, True } Boolean;

EXPORT typedef unsigned char *string;

EXPORT typedef struct _pairlist {
  unsigned char *name, *value;
  struct _pairlist *next;
} *pairlist;

EXPORT typedef unsigned int MediaSet;
EXPORT enum _Media {
  MediaNone = 0,
  MediaPrint = (1 << 0),
  MediaScreen = (1 << 1),
  MediaTTY = (1 << 2),
  MediaBraille = (1 << 3),
  MediaTV = (1 << 4),
  MediaProjection = (1 << 5),
  MediaEmbossed = (1 << 6),
  MediaAll = 0xFF
};

#define eq(s, t) (*s == *t && strcmp(s, t) == 0)
EXPORTDEF(eq(s, t))

#define hexval(c) ((c) <= '9' ? (c)-'0' : (c) <= 'F' ? 10+(c)-'A' : 10+(c)-'a')
EXPORTDEF(hexval(c))

/* pairlist_delete -- free all memory occupied by a pairlist */
EXPORT void pairlist_delete(pairlist p)
{
  if (p) {
    pairlist_delete(p->next);
    dispose(p->name);
    dispose(p->value);
    dispose(p);
  }
}

/* pairlist_copy -- make a deep copy of a pairlist */
EXPORT pairlist pairlist_copy(const pairlist p)
{
  pairlist h = NULL;

  if (p) {
    new(h);
    h->name = newstring(p->name);
    h->value = newstring(p->value);
    h->next = pairlist_copy(p->next);
  }
  return h;
}

/* strapp -- append to a string, re-allocating memory; last arg must be 0 */
EXPORT string strapp(string *s,...)
{
  va_list ap;
  int i, j;
  string h;

  va_start(ap, s);
  i = *s ? strlen(*s) : 0;
  while ((h = va_arg(ap, string))) {
    j = strlen(h);
    renewarray(*s, i + j + 1);
    strcpy(*s + i, h);
    i += j;
  }
  va_end(ap);
  return *s;
}

/* chomp -- remove trailing \n (if any) from string */
EXPORT void chomp(string s)
{
  int i;

  if (s && (i = strlen(s)) != 0 && s[i-1] == '\n') s[i-1] = '\0';
}

EXPORT inline int min(int a, int b) { return a < b ? a : b; }
EXPORT inline int max(int a, int b) { return a > b ? a : b; }
