/*
 * incl - expand included files
 *
 * Searches for <!--include "file"--> and expands the referenced file
 * in place. File may be a URL. Works recursively. Other acepted
 * syntaxes:
 *
 * <!--include "file"-->
 * <!--include 'file'-->
 * <!--include file-->
 * <!--begin-include "file"-->...<!--end-include-->
 * <!--begin-include 'file'-->...<!--end-include-->
 * <!--begin-include file-->...<!--end-include-->
 *
 * If there are no quotes, the file name may not include whitespace.
 *
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos
 * Created: 2 Dec 1998
 * Version: $Id: incl.c,v 1.10 2003/09/04 14:18:46 bbos Exp $
 *
 **/
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
# ifndef HAVE_STRSTR
#  include "strstr.e"
# endif
#endif
#include <stdlib.h>
#include <assert.h>
#include "export.h"
#include "types.e"
#include "html.e"
#include "scan.e"
#include "openurl.e"
#include "heap.e"
#include "url.e"

#define INCLUDE "include"
#define BEGIN "begin-include"
#define END "end-include"

typedef struct _stack {
  Boolean skipping;
  struct _stack *next;
} *stack;

typedef enum {KNone, KIncl, KBegin, KEnd} Key;

extern int yylineno;				/* From scan.l */

static Boolean do_xml = False;
static Boolean has_error = False;
static string base = NULL;
static stack skipping = NULL;


/* push -- push a skipping state on the stack */
static void push(stack *skipping, Boolean s)
{
  stack h;

  new(h);
  h->next = *skipping;
  h->skipping = s;
  *skipping = h;
}

/* pop -- pop a skipping state off the stack */
static void pop(stack *skipping)
{
  stack h;

  assert(*skipping);
  h = *skipping;
  *skipping = (*skipping)->next;
  dispose(h);
}

/* top -- return value of top of skipping stack */
static Boolean top(stack skipping)
{
  assert(skipping);
  return skipping->skipping;
}

/* word_to_key -- check whether word s is one of the recognized keywords */
static Key word_to_key(const string s, int len)
{
  if (len == sizeof(END) - 1 && strncmp(s, END, len)== 0) return KEnd;
  if (len == sizeof(INCLUDE) - 1 && strncmp(s, INCLUDE, len)== 0) return KIncl;
  if (len == sizeof(BEGIN) - 1 && strncmp(s, BEGIN, len)== 0) return KBegin;
  return KNone;
}

/* --------------- implements interface api.h -------------------------- */

/* handle_error -- called when a parse error occurred */
void handle_error(void *clientdata, const string s, int lineno)
{
  fprintf(stderr, "%d: %s\n", lineno, s);
  has_error = True;
}

/* start -- called before the first event is reported */
void* start(void)
{
  push(&skipping, False);			/* Start by not skipping */
  return NULL;
}
  
/* end -- called after the last event is reported */
void end(void *clientdata)
{
  assert(clientdata == NULL);
  assert(top(skipping) == False);
  /* skip */
}

/* handle_comment -- called after a comment is parsed */
void handle_comment(void *clientdata, string commenttext)
{
  /* A push() occurs at <!--begin-include...--> and at include_file() */
  /* A pop() occurs at <!--end-include...--> and at ENDINCL */
  int i, j;
  string s, url;
  FILE *f;
  Key key;

  i = strspn(commenttext, " \t\n\r\f");		/* Skip whitespace */
  j = strcspn(commenttext + i, " \t\n\r\f");	/* First word */
  key = word_to_key(commenttext + i, j);
  
  if (key == KEnd) {				/* <!--end-include...--> */

    /* Don't print anything, just pop a level */
    pop(&skipping);

  } else if (top(skipping)) {			/* Are we already skipping? */

    /* Don't print anything; push a level if this is a begin-include */
    if (key == KBegin) push(&skipping, True);

  } else if (key == KNone) {			/* Unrecognized comment? */

    /* Print the comment verbatim */
    printf("<!--%s-->", commenttext);

  } else {					/* include or begin-include */

    /* Push a level if this is a begin-include */
    if (key == KBegin) push(&skipping, True);

    /* Find start of file name */
    i += j;
    i += strspn(commenttext + i, " \t\n\r\f");	/* Skip whitespace */

    /* Accept either "...", '...", or any string without spaces */
    if (commenttext[i] == '"') {
      j = strcspn(commenttext + i + 1, "\"");
      url = newnstring(commenttext + i + 1, j);
    } else if (commenttext[i] == '\'') {
      j = strcspn(commenttext + (++i), "'");
      url = newnstring(commenttext + i + 1, j);
    } else {
      j = strcspn(commenttext + i, " \t\n\r\f");
      url = newnstring(commenttext + i, j);
    }

    /* Get the file and recursively parse it */
    s = URL_s_absolutize(base, url);
    if (!(f = fopenurl(s, "r")))
      perror(url);
    else {
      printf("<!--%s %s-->", BEGIN, commenttext + i);
      push(&skipping, False);
      include_file(f);
    }
    dispose(url);
    dispose(s);
  }

  free(commenttext);
}

/* handle_text -- called after a text chunk is parsed */
void handle_text(void *clientdata, string text)
{
  if (top(skipping) == False) printf("%s", text);
  free(text);
}

/* handle_decl -- called after a declaration is parsed */
void handle_decl(void *clientdata, string gi, string fpi, string url)
{
  printf("<!DOCTYPE %s", gi);
  if (!fpi) printf(" SYSTEM \"%s\">", url);
  else if (!url) printf(" PUBLIC \"%s\">", fpi);
  else printf(" PUBLIC \"%s\" \"%s\">", fpi, url);
  free(gi);
  free(fpi);
  free(url);
}

/* handle_pi -- called after a PI is parsed */
void handle_pi(void *clientdata, string pi_text)
{
  if (top(skipping) == False) printf("<?%s>", pi_text);
  free(pi_text);
}

/* handle_starttag -- called after a start tag is parsed */
void handle_starttag(void *clientdata, string name, pairlist attribs)
{
  pairlist p;

  if (top(skipping) == False) {
    printf("<%s", name);
    for (p = attribs; p; p = p->next) {
      if (p->value != NULL) printf(" %s=\"%s\"", p->name, p->value);
      else if (do_xml) printf(" %s=\"%s\"", p->name, p->name);
      else printf(" %s", p->name);
    }
    printf(">");
  }
  free(name);
  pairlist_delete(attribs);
}

/* handle_emptytag -- called after an empty tag is parsed */
void handle_emptytag(void *clientdata, string name, pairlist attribs)
{
  pairlist p;

  if (top(skipping) == False) {
    printf("<%s", name);
    for (p = attribs; p; p = p->next) {
      if (p->value != NULL) printf(" %s=\"%s\"", p->name, p->value);
      else if (do_xml) printf(" %s=\"%s\"", p->name, p->name);
      else printf(" %s", p->name);
    }
    printf(do_xml ? " />" : ">");
  }
  free(name);
  pairlist_delete(attribs);
}

/* handle_endtag -- called after an endtag is parsed (name may be "") */
void handle_endtag(void *clientdata, string name)
{
  if (top(skipping) == False) printf("</%s>", name);
  free(name);
}

/* handle_endincl -- called after the end of an included file is reached */
void handle_endincl(void *clientdata)
{
  pop(&skipping);

  /* Mark the end of the inclusion */
  printf("<!--%s-->", END);
}

/* --------------------------------------------------------------------- */

/* usage -- print usage message and exit */
static void usage(string prog)
{
  fprintf(stderr, "Version %s\nUsage: %s [-x] [-b base] [file-or-url]\n",
	  VERSION, prog);
  exit(2);
}

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
  set_endincl_handler(handle_endincl);

  /* Parse command line arguments */
  for (i = 1; i < argc && argv[i][0] == '-' && !eq(argv[i], "--"); i++) {
    switch (argv[i][1]) {
      case 'x': do_xml = True; break;
      case 'b': base = argv[++i]; break;
      default: usage(argv[0]);
    }
  }
  if (i < argc && eq(argv[i], "--")) i++;

  if (i == argc) {
    yyin = stdin;
    base = ".";
  } else if (i == argc - 1) {
    yyin = fopenurl(argv[i], "r");
    if (!base) base = argv[i];
  } else {
    usage(argv[0]);
  }

  if (yyin == NULL) {perror(argv[i]); exit(1);}

  if (yyparse() != 0) exit(3);

  return has_error ? 1 : 0;
}
