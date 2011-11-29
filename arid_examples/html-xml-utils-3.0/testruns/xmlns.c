/*
 * xmlns - expand XML Namespace prefixes
 *
 * Expand all element and attribute names to "global names" by
 * expanding the prefix. All names will be printed as "{URL}name".
 * Attribute names without a prefix will have an empty namespace part:
 * "{}name".
 *
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos
 * Created: 22 Mar 2000
 * Version: $Id: xmlns.c,v 1.7 2000/08/21 11:36:05 bbos Exp $
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
#endif
#include <stdlib.h>
#include <assert.h>
#include "export.h"
#include "types.e"
#include "heap.e"
#include "html.e"
#include "scan.e"
#include "openurl.e"

extern int yylineno;				/* From scan.l */

/* The symbol table is a chain of prefix/uri pairs. Every time an
 * element starts, the prefixes defined by it are added at the end. To
 * expand a prefix, the most recently added prefix/uri pair is used.
 * When en element ends, the chain is reduced to what it was when the
 * element started. The stack keeps track of where the chain ended at
 * the start of the element.
 *
 * ToDo: should we hash the prefixes? or is linear search good enough?
 **/
typedef struct _Symbol {
  string prefix;
  string uri;
  struct _Symbol *next;
} Symbol, *SymbolTable;

typedef struct _StackElt {
  Symbol *frame;
  struct _StackElt *next;
} *Stack;

static Symbol xml = {"xml", "http://www.w3.org/XML/1998/namespace", NULL};
static Boolean has_error = False;
static SymbolTable symtable = &xml;
static Stack stack = NULL;
static Boolean do_decls = True;			/* Print decl, comment, PI? */


/* print_globalname -- print a name with expanded prefix */
static void print_globalname(string name, Boolean use_default)
{
  string h, prefix, local;
  Symbol *s;

  /* Split the name */
  h = strchr(name, ':');
  if (!h && !use_default) {			/* No prefix & no default ns */
    printf("{}%s", name);
    return;
  }
  if (h) {
    *h = '\0';
    prefix = name;
    local = h + 1;
  } else {
    prefix = "";
    local = name;
  }
  /* Find the prefix in the symbol table */
  for (s = symtable; s && !eq(prefix, s->prefix); s = s->next) ;

  if (!s) {
    fprintf(stderr, "%d: prefix \"%s\" not defined\n", yylineno, prefix);
    has_error = True;
  }
  /* ToDo: check that any '}' in uri is escaped */
  printf("{%s}%s", s ? s->uri : (string)"", local);
}

/* do_tag -- print a start or empty tag expanded */
static void do_tag(string name, pairlist attribs, Boolean empty)
{
  Stack h;
  pairlist p;
  Symbol *sym;

  /* Mark the current end of the symbol table */
  new(h);
  h->next = stack;
  h->frame = symtable;
  stack = h;

  /* Scan the attributes for namespace definitions and store them */
  for (p = attribs; p; p = p->next) {
    if (strncmp(p->name, "xmlns", 5) == 0) {
      new(sym);
      sym->prefix = newstring(p->name + (p->name[5] ? 6 : 5));
      sym->uri = newstring(p->value);
      sym->next = symtable;
      symtable = sym;
    }
  }
  /* Print the tag with prefixes expanded */
  putchar('<');
  print_globalname(name, True);
  for (p = attribs; p; p = p->next) {
    if (strncmp(p->name, "xmlns", 5) != 0) {
      putchar(' ');
      print_globalname(p->name, False);
      printf("=\"%s\"", p->value);
    }
  }
  printf(empty ? "/>" : ">");
}

/* pop_symboltable -- unwind the symbol table to previous mark */
static void pop_symboltable(string name)
{
  Symbol *h;
  Stack p;

  if (!stack) {
    if (! has_error) fprintf(stderr, "%d: too many end tags\n", yylineno);
    has_error = True;
    return;
  }
  /* Remove entries from symbol table chain until last mark */
  while (symtable != stack->frame) {
    h = symtable;
    symtable = symtable->next;
    dispose(h->prefix);
    dispose(h->uri);
    dispose(h);
  }
  /* Pop stack itself */
  p = stack;
  stack = stack->next;
  dispose(p);
}

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
  /* skip */
}

/* handle_comment -- called after a comment is parsed */
void handle_comment(void *clientdata, string commenttext)
{
  if (do_decls) printf("<!--%s-->", commenttext);
  free(commenttext);
}

/* handle_text -- called after a text chunk is parsed */
void handle_text(void *clientdata, string text)
{
  printf("%s", text);
  free(text);
}

/* handle_decl -- called after a declaration is parsed */
void handle_decl(void *clientdata, string gi, string fpi, string url)
{
  if (do_decls) {
    if (!fpi) printf("<!DOCTYPE %s SYSTEM \"%s\">", gi, url);
    else if (!url) printf("<!DOCTYPE %s PUBLIC \"%s\">", gi, fpi);
    else printf("<!DOCTYPE %s PUBLIC \"%s\" \"%s\">", gi, fpi, url);
  }
  free(gi);
  if (fpi) free(fpi);
  if (url) free(url);
}

/* handle_pi -- called after a PI is parsed */
void handle_pi(void *clientdata, string pi_text)
{
  if (do_decls) printf("<?%s>", pi_text);
  free(pi_text);
}

/* handle_starttag -- called after a start tag is parsed */
void handle_starttag(void *clientdata, string name, pairlist attribs)
{
  do_tag(name, attribs, False);
  free(name);
  pairlist_delete(attribs);
}

/* handle_emptytag -- called after an empty tag is parsed */
void handle_emptytag(void *clientdata, string name, pairlist attribs)
{
  do_tag(name, attribs, True);
  pop_symboltable(name);
  free(name);
  pairlist_delete(attribs);
}

/* handle_endtag -- called after an endtag is parsed (name may be "") */
void handle_endtag(void *clientdata, string name)
{
  /* Printf the end tag */
  printf("</");
  print_globalname(name, True);
  putchar('>');
  
  /* Unwind the symbol table */
  pop_symboltable(name);
  free(name);
}

/* usage -- print usage message and exit */
static void usage(string prog)
{
  fprintf(stderr, "Version %s\nUsage: %s [-d] [xml-file-or-url]\n", VERSION, prog);
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
      case 'd': do_decls = False; break;
      default: usage(argv[0]);
    }
  }
  if (i < argc && eq(argv[i], "--")) i++;

  if (i == argc) yyin = stdin;
  else if (i == argc - 1) yyin = fopenurl(argv[i], "r");
  else usage(argv[0]);

  if (yyin == NULL) {perror(argv[i]); exit(1);}

  if (yyparse() != 0) exit(3);

  return has_error ? 1 : 0;
}
