/*
 * Count elements and attributes.
 *
 * This counts occurrences of elements and element/attribute pairs.
 * This is just an example of how to use the parser.
 * No attempt is made to count efficiently.
 *
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Bert Bos
 * Created Nov 1998
 * $Id: count.c,v 1.12 2000/08/07 12:28:44 bbos Exp $
 */
#include <config.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <ctype.h>
#if STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
# ifndef HAVE_STRdup
#  include "strdup.e"
# endif
#endif
#include <stdlib.h>
#include <assert.h>
#include "export.h"
#include "types.e"
#include "html.e"
#include "scan.e"
#include "openurl.e"

typedef struct _pair {
  char *name;
  int count;
} pair;

static pair *freq = NULL;
static int nrelems = 0;
static Boolean has_errors = False;


/* countstring -- add 1 to number of occurences for s (case-insensitively) */
static void countstring(char *s)
{
  int i;

  i = 0;
  while (i < nrelems && strcasecmp(freq[i].name, s) != 0) i++;
  if (i == nrelems) {
    nrelems++;
    freq = realloc(freq, nrelems * sizeof(freq[0]));
    if (freq == NULL) {fprintf(stderr, "Out of memory\n"); exit(4);}
    freq[i].name = strdup(s);
    freq[i].count = 0;
  }
  freq[i].count++;
}

/* count -- count element types and their attributes */
static void count(char *name, pairlist attribs)
{
  /* Count element name */
  countstring(name);

  /* Count attribute names (or rather, the strings "elem/attrib") */
  for (; attribs != NULL; attribs = attribs->next) {
    char *s = malloc(strlen(name) + strlen(attribs->name) + 2);
    if (s == NULL) {fprintf(stderr, "Out of memory\n"); exit(4);}
    strcat(strcat(strcpy(s, name), "/"), attribs->name);
    countstring(s);
    free(s);
  }
}

/* handle_error -- called when a parse error occurred */
void handle_error(void *clientdata, const string s, int lineno)
{
  fprintf(stderr, "%d: %s\n", lineno, s);
  has_errors = True;
}

/* start -- called before the first event is reported */
void* start(void) {return NULL;}

/* end -- called after the last event is reported */
void end(void *clientdata) {}

/* handle_comment -- called after a comment is parsed */
void handle_comment(void *clientdata, string commenttext) {}

/* handle_text -- called after a tex chunk is parsed */
void handle_text(void *clientdata, string text) {}

/* handle_declaration -- called after a declaration is parsed */
void handle_decl(void *clientdata, string gi,
		 string fpi, string url) {}

/* handle_proc_instr -- called after a PI is parsed */
void handle_pi(void *clientdata, string pi_text) {}

/* handle_starttag -- called after a start tag is parsed */
void handle_starttag(void *clientdata, string name, pairlist attribs)
{
  count(name, attribs);
}

/* handle_emptytag -- called after am empty tag is parsed */
extern void handle_emptytag(void *clientdata, string name, pairlist attribs)
{
  count(name, attribs);
}

/* handle_pop -- called after an endtag is parsed (name may be "") */
extern void handle_endtag(void *clientdata, string name) {}

/* usage -- print usage message and exit */
static void usage(string prog)
{
  fprintf(stderr, "Version %s\n", VERSION);
  fprintf(stderr, "Usage: %s [html-file]\n", prog);
  exit(2);
}

/* main -- parse input, count elements and attributes of each type */
int main(int argc, char *argv[])
{
  int i;

  /* Bind the parser callback routines to our handlers */
  set_error_handler(handle_error);
  set_start_handler(start);
  set_end_handler(end);
  set_comment_handler(handle_comment);
  set_text_handler(handle_text);
  set_decl_handler(handle_decl);
  set_pi_handler(handle_pi);
  set_starttag_handler(handle_starttag);
  set_emptytag_handler(handle_emptytag);
  set_endtag_handler(handle_endtag);

  if (argc == 1) yyin = stdin;
  else if (argc == 2) yyin = fopenurl(argv[1], "r");
  else usage(argv[0]);

  if (yyin == NULL) {perror(argv[1]); exit(1);}

  /* Parse input */
  if (yyparse() != 0) exit(3);

  /* Print results */
  for (i = 0; i < nrelems; i++)
    printf("%6d\t%s\n", freq[i].count, freq[i].name);

  return has_errors ? 1 : 0;
}
