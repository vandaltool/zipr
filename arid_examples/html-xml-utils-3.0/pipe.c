/*
 * pipe - output HTML/XML in canonical ("sgmls" form
 *
 * Parse HTML/XML and output in approximate "nsgmls" format. Some of
 * the differences are that comments are also printed (see * below),
 * that implied attributes are not, and that entities are left
 * unexpanded. Use "unent" to expand entities to UTF-8.
 *
 * The program doesn't interpret the source in any way, and doesn't
 * read DTDs. That means that, e.g., end tags are not automatically
 * added. Pipe the source through normalize(1) first in order to
 * convert HTML to XML and infer missing tags.
 *
 * The possible command characters and arguments are as follows:
 *
 *     (gi
 *
 *	    The start of an element whose generic identifier is gi.
 *	    Any attributes for this element will have been speci- fied
 *	    with A commands.
 *
 *     )gi
 *
 *	    The end of an element whose generic identifier is gi.
 *
 *     |gi
 *
 *	    An empty element (an element whose tag in the source ended
 *	    with a slash). Any attributes will have been specified
 *	    with A commands. (Note that this distinguishes empty
 *	    elements from elements that happen to have no content,
 *	    even though XML doesn't.)
 *
 *     -data
 *
 *	    Data.
 *
 *     ?pi
 *
 *	    A processing instruction with data pi.
 *
 *     *comment
 *
 *	    A comment
 *
 *     Aname type val
 *
 *	    The next element to start has an attribute name with value
 *	    val and type type. Implied attribute are not shown. All
 *	    attributes are assumed to be of type CDATA, since pipe
 *	    doesn't read DTDs.
 *
 *     !root "fpi" url
 *     !root "fpi"
 *     !root "" url
 *
 *	    A document type declaration. The fpi (public identifier)
 *	    is a quoted string. If there is no fpi, the string is
 *	    empty: "". If there is no url, itis omitted.
 *
 *     Llineno
 *
 *	    Set the current line number. This will be output only if
 *	    the -l option has been given.
 *
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos
 * Created: 2 Dec 1998
 * Version: $Id: pipe.c,v 1.14 2000/08/07 12:28:44 bbos Exp $
 *
 **/
#include <config.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <ctype.h>
#if STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
#endif
#include <stdlib.h>
#include <assert.h>
#include "export.h"
#include "types.e"
#include "html.e"
#include "scan.e"
#include "openurl.e"

extern int yylineno;				/* From scan.l */

static Boolean has_error = False;
static Boolean in_text = False;
static Boolean linenumbering = False;


/* escape -- print a string with certain characters escaped */
static void escape(const string t)
{
  string s;

  for (s = t; *s; s++)
    switch (*s) {
      case '\r': printf("\\r"); break;
      case '\t': printf("\\t"); break;
      case '\n': printf("\\n"); break;
      case '\\': printf("\\\\"); break;
      default: putchar(*s);
    }
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
  return NULL;
}
  
/* end -- called after the last event is reported */
void end(void *clientdata)
{
  if (in_text) {putchar('\n'); in_text = False;}
}

/* handle_comment -- called after a comment is parsed */
void handle_comment(void *clientdata, string commenttext)
{
  if (in_text) {putchar('\n'); in_text = False;}
  if (linenumbering) printf("L%d\n", yylineno);
  putchar('*');
  escape(commenttext);
  putchar('\n');
}

/* handle_text -- called after a text chunk is parsed */
void handle_text(void *clientdata, string text)
{
  /* There may be several consecutive calls to this routine. The
   * variable 'in_text' is used to put the text of all of them on the
   * same line.
   **/
  if (! in_text) {
    if (linenumbering) printf("L%d\n", yylineno);
    putchar('-');
    in_text = True;
  }
  escape(text);
}

/* handle_decl -- called after a declaration is parsed */
void handle_decl(void *clientdata, string gi, string fpi,
		 string url)
{
  if (in_text) {putchar('\n'); in_text = False;}
  if (linenumbering) printf("L%d\n", yylineno);
  putchar('!');
  printf("%s \"%s\"", gi, fpi);
  if (url) printf(" %s", url);
  putchar('\n');
}

/* handle_pi -- called after a PI is parsed */
void handle_pi(void *clientdata, string pi_text)
{
  if (in_text) {putchar('\n'); in_text = False;}
  if (linenumbering) printf("L%d\n", yylineno);
  putchar('?');
  escape(pi_text);
  putchar('\n');
}

/* handle_starttag -- called after a start tag is parsed */
void handle_starttag(void *clientdata, string name, pairlist attribs)
{
  pairlist p;

  if (in_text) {putchar('\n'); in_text = False;}
  for (p = attribs; p; p = p->next) {
    putchar('A');
    /* down(p->name); */
    printf("%s", p->name);
    if (strcmp(p->name, "id") == 0) printf(" ID "); else printf(" CDATA ");
    if (p->value) escape(p->value); else printf("%s", p->name);
    putchar('\n');
  }
  if (linenumbering) printf("L%d\n", yylineno);
  putchar('(');
  printf("%s", name);
  putchar('\n');
}

/* handle_emptytag -- called after an empty tag is parsed */
void handle_emptytag(void *clientdata, string name, pairlist attribs)
{
  pairlist p;

  if (in_text) {putchar('\n'); in_text = False;}
  for (p = attribs; p; p = p->next) {
    putchar('A');
    /* down(p->name); */
    printf("%s", p->name);
    if (strcmp(p->name, "id") == 0) printf(" ID "); else printf(" CDATA ");
    if (p->value) escape(p->value); else printf("%s", p->name);
    putchar('\n');
  }
  if (linenumbering) printf("L%d\n", yylineno);
  putchar('|');
  printf("%s", name);
  putchar('\n');
}

/* handle_endtag -- called after an endtag is parsed (name may be "") */
void handle_endtag(void *clientdata, string name)
{
  if (in_text) {putchar('\n'); in_text = False;}
  if (linenumbering) printf("L%d\n", yylineno);
  putchar(')');
  printf("%s", name);
  putchar('\n');
}

/* --------------------------------------------------------------------- */

/* usage -- print usage message and exit */
static void usage(string prog)
{
  fprintf(stderr, "Version %s\nUsage: %s [-l] [html-file-or-url]\n", VERSION, prog);
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

  /* Parse command line arguments */
  for (i = 1; i < argc && argv[i][0] == '-' && !eq(argv[i], "--"); i++) {
    switch (argv[i][1]) {
      case 'l': linenumbering = True;; break;
      default: usage(argv[0]);
    }
  }
  if (i < argc && eq(argv[i], "--")) i++;

  if (i == argc) yyin = stdin;
  else if (i == argc - 1 && eq(argv[i], "-")) yyin = stdin;
  else if (i == argc - 1) yyin = fopenurl(argv[i], "r");
  else usage(argv[0]);

  if (yyin == NULL) {perror(argv[i]); exit(1);}

  if (yyparse() != 0) exit(3);

  return has_error ? 1 : 0;
}
