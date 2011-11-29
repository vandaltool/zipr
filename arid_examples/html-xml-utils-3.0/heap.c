/*
 * Copyright © 1994-2003 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: before 1995
 * Version: $Id: heap.c,v 1.7 2003/04/09 10:07:12 bbos Exp $
 **/
#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#if STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
#endif
#include "export.h"

#ifdef __export
//#define FILE / ## *"*/__FI ## LE__/*"* ## /
//#define LINE / ## *"*/__LI ## NE__/*"* ## /
#undef __FILE__			/* Don't expand while making the .e file */
#undef __LINE__			/* Don't expand while making the .e file */
#endif

#define fatal(msg) fatal3(msg, __FILE__, __LINE__)
#define new(p) if (((p)=malloc(sizeof(*(p))))); else fatal("out of memory")
#define dispose(p) if (!(p)) ; else (free(p), (p) = (void*)0) 
#define heapmax(p) 9999999 /* ? */
#define newstring(s) heap_newstring(s, __FILE__, __LINE__) 
#define newnstring(s,n) heap_newnstring(s, n, __FILE__, __LINE__) 
#define newarray(p,n) \
    if (((p)=malloc((n)*sizeof(*(p))))); else fatal("out of memory")
#define renewarray(p,n) \
    if (((p)=realloc(p,(n)*sizeof(*(p))))); else fatal("out of memory")

EXPORTDEF(fatal(msg))
EXPORTDEF(new(p))
EXPORTDEF(dispose(p))
EXPORTDEF(heapmax(p))
EXPORTDEF(newstring(s))
EXPORTDEF(newnstring(s,n))
EXPORTDEF(newarray(p,n))
EXPORTDEF(renewarray(p,n))


EXPORT void fatal3(const char *s, const char *file, const int line)
{
    fprintf(stderr, "%s (file %s, line %d)\n", s, file, line);
    abort();
}


EXPORT char * heap_newstring(const char *s, const char *file, const int line)
{
    char *t;

    if (!s) return NULL;
    t = malloc((strlen(s) + 1) * sizeof(*t));
    if (!t) fatal3("out of memory", file, line);
    strcpy(t, s);
    return t;
}

EXPORT char * heap_newnstring(const char *s, const size_t n,
			      const char *file, const int line)
{
    char *t;

    if (!s) return NULL;
    t = malloc((n + 1) * sizeof(*t));
    if (!t) fatal3("out of memory", file, line);
    strncpy(t, s, n);
    t[n] = '\0';
    return t;
}
