/*
 * Add an ID to selected elements
 *
 * Copyright © 2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 20 Aug 2000
 * Version: $Id: addid.c,v 1.4 2003/08/07 20:57:20 bbos Exp $
 *
 **/
#include <config.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
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

#ifdef HAVE_ERRNO_H
#  include <errno.h>
#endif
#include "export.h"
#include "types.e"
#include "heap.e"
#include "tree.e"
#include "html.e"
#include "scan.e"
#include "openurl.e"
#include "errexit.e"
#include "genid.e"
#include "class.e"

static Tree tree;
static Boolean xml = False;			/* Use <empty /> convention */
static string targetelement = NULL;		/* Element to extract */
static string targetclass = NULL;		/* Class to extract */


/* is_match check whether the element matches the target element and class */
static Boolean is_match(const string name, pairlist attribs)
{
  return ((!targetelement || strcasecmp(name, targetelement) == 0)
	  && (!targetclass || has_class(attribs, targetclass)));
}

/* handle_error -- called when a parse error occurred */
EXPORT void handle_error(void *clientdata, const string s, int lineno)
{
  fprintf(stderr, "%d: %s\n", lineno, s);
}

/* start -- called before the first event is reported */
EXPORT void* start(void)
{
  tree = create();
  return NULL;
}
  
/* end -- called after the last event is reported */
EXPORT void end(void *clientdata)
{
  /* skip */
}

/* handle_comment -- called after a comment is parsed */
EXPORT void handle_comment(void *clientdata, string commenttext)
{
  tree = append_comment(tree, commenttext);
}

/* handle_text -- called after a tex chunk is parsed */
EXPORT void handle_text(void *clientdata, string text)
{
  tree = append_text(tree, text);
}

/* handle_declaration -- called after a declaration is parsed */
EXPORT void handle_decl(void *clientdata, string gi,
			string fpi, string url)
{
  tree = append_declaration(tree, gi, fpi, url);
}

/* handle_proc_instr -- called after a PI is parsed */
EXPORT void handle_pi(void *clientdata, string pi_text)
{
  tree = append_procins(tree, pi_text);
}

/* handle_starttag -- called after a start tag is parsed */
EXPORT void handle_starttag(void *clientdata, string name, pairlist attribs)
{
  pairlist p;

  tree = html_push(tree, name, attribs);

  /* If it has an ID, store it (so we don't accidentally generate it) */
  for (p = attribs; p; p = p->next)
    if (strcasecmp(p->name, "id") == 0 && p->value) storeID(p->value);
}

/* handle_emptytag -- called after an empty tag is parsed */
EXPORT void handle_emptytag(void *clientdata, string name, pairlist attribs)
{
  handle_starttag(clientdata, name, attribs);
}

/* handle_endtag -- called after an endtag is parsed (name may be "") */
EXPORT void handle_endtag(void *clientdata, string name)
{
  tree = html_pop(tree, name);
}

/* expand -- write the tree, inserting ID's at matching elements */
static void expand(Tree t)
{
  Tree h;
  pairlist a;

  for (h = t->children; h != NULL; h = h->sister) {
    switch (h->tp) {
      case Text: printf("%s", h->text); break;
      case Comment: printf("<!--%s-->", h->text); break;
      case Declaration:
	printf("<!DOCTYPE %s", h->name);
	if (h->text) printf(" PUBLIC \"%s\"", h->text);	else printf(" SYSTEM");
	if (h->url) printf(" \"%s\"", h->url);
	printf(">");
	break;
      case Procins: printf("<?%s>", h->text); break;
      case Element:
	if (is_match(h->name, h->attribs) && !get_attrib(h, "id", NULL))
	  set_attrib(h, "id", gen_id(h));
	printf("<%s", h->name);
	for (a = h->attribs; a != NULL; a = a->next) {
	  printf(" %s", a->name);
	  if (a->value != NULL) printf("=\"%s\"", a->value);
	}
	if (is_empty(h->name)) {
	  printf(xml ? " />" : ">");
	} else {
	  printf(">");
	  expand(h);
	  printf("</%s>", h->name);
	}
	break;
      case Root: assert(! "Cannot happen"); break;
      default: assert(! "Cannot happen");
    }
  }
}

/* usage -- print usage message and exit */
static void usage(string name)
{
  errexit("Version %s\n\
Usage: %s [-x] [--] elem|.class|elem.class [html-file]\n",
	  VERSION, name);
}


int main(int argc, char *argv[])
{
  char * p;
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

  /* Parse command line options */
  for (i = 1; i < argc && argv[i][0] == '-' && !eq(argv[i], "--"); i++) {
    switch (argv[i][1]) {
      case 'x': xml = True; break;
      default: usage(argv[0]);
    }
  }
  if (i < argc && eq(argv[i], "--")) i++;

  if (i == argc) usage(argv[0]);
  if (argv[i][0] == '.') {			/* Class name */
    targetclass = argv[i] + 1;
  } else {					/* Element name */
    targetelement = argv[i];
    if ((p = strchr(targetelement, '.'))) {
      *p = '\0';
      targetclass = p + 1;
    }
  }
  i++;
  if (i == argc) yyin = stdin;
  else if (i == argc - 1 && eq(argv[i], "-")) yyin = stdin;
  else if (i == argc - 1) yyin = fopenurl(argv[i], "r");
  else usage(argv[0]);

  if (yyin == NULL) {perror(argv[i]); exit(1);}

  if (yyparse() != 0) exit(3);

  tree = get_root(tree);
  expand(tree);
  tree_delete(tree);				/* Just to test memory mgmt */
  return 0;
}
