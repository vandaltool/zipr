/*
 * Insert an active ToC between "<!--begin-toc-->" and "<!--end-toc-->",
 * or replacing the comment "<!--toc-->"
 *
 * Headers with class "no-toc" will not be listed in the ToC.
 *
 * The ToC links to elements with ID attributes as well as with
 * empty <A NAME> elements.
 *
 * Tags for a <SPAN> with class "index" are assumed to be used by
 * a cross-reference generator and will not be copied to the ToC.
 *
 * Any <A> tags with a class of "bctarget" are not copied, but
 * regenerated. They are assumed to be backwards-compatible versions
 * of ID attributes on their parent elements. With the option -t or -x
 * they are removed.
 *
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created Sep 1997
 * Version: $Id: toc.c,v 1.34 2003/04/09 10:07:12 bbos Exp $
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
#ifdef HAVE_SEARCH_H
#  include <search.h>
#else
#  include "search-freebsd.h"
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

#define BEGIN_TOC "begin-toc"			/* <!--begin-toc--> */
#define END_TOC "end-toc"			/* <!--end-toc--> */
#define TOC "toc"				/* <!--toc--> */
#define NO_TOC "no-toc"				/* CLASS="... no-toc..." */
#define INDEX "index"				/* CLASS="... index..." */
#define TARGET "bctarget"			/* CLASS="...bctarget..." */

#define EXPAND True
#define NO_EXPAND False
#define KEEP_ANCHORS True
#define REMOVE_ANCHORS False

#define INDENT " "				/* Amount to indent ToC per level */

static Tree tree;
static int toc_low = 1, toc_high = 6;		/* Which headers to include */
static Boolean xml = False;			/* Use <empty /> convention */
static Boolean bctarget = True;			/* Generate <a name=> after IDs */


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

/* indent -- print level times a number of spaces */
static void indent(int level)
{
  for (; level > 0; level--) printf(INDENT);
}

static void expand(Tree t, Boolean *write, Boolean exp, Boolean keep_anchors);

/* toc -- create a table of contents */
static void toc(Tree t, int *curlevel, Boolean *item_is_open)
{
  int level;
  Tree h;
  unsigned char *id, *val;
  Boolean write = True;

  switch (t->tp) {
    case Text: break;
    case Comment: break;
    case Declaration: break;
    case Procins: break;
    case Element:
      /* Check if the element is a heading */
      if (eq(t->name, "h1")) level = 1;
      else if (eq(t->name, "h2")) level = 2;
      else if (eq(t->name, "h3")) level = 3;
      else if (eq(t->name, "h4")) level = 4;
      else if (eq(t->name, "h5")) level = 5;
      else if (eq(t->name, "h6")) level = 6;
      else level = -1;
      /* Check if it has a "no-toc" class */
      if (level > 0 && get_attrib(t, "class", &val) && contains(val, NO_TOC))
	level = -1;
      /* If it's a header for the ToC, create a list item for it */
      if (level >= toc_low && level <= toc_high) {
	/* Ensure there is an ID to point to */
	if (! get_attrib(t, "id", NULL)) set_attrib(t, "id", gen_id(t));
	assert(*curlevel <= level || *item_is_open);
	while (*curlevel > level) {
	  printf(xml ? "</li>\n" : "\n");
	  indent(*curlevel - toc_low);
	  printf("</ul>");
	  (*curlevel)--;
	}
	if (*curlevel == level && *item_is_open) {
	  printf(xml ? "</li>\n" : "\n");
	} else if (*item_is_open) {
	  printf("\n");
	  (*curlevel)++;
	  indent(*curlevel - toc_low);
	  printf("<ul class=\"%s\">\n", TOC);
	}
	while (*curlevel < level) {
	  indent(*curlevel - toc_low);
 	  printf("<li>\n");
	  (*curlevel)++;
	  indent(*curlevel - toc_low);
	  printf("<ul class=\"%s\">\n", TOC);
	}
	(void) get_attrib(t, "id", &id);
	indent(*curlevel - toc_low);
	printf("<li><a href=\"#%s\">", id);
	expand(t, &write, NO_EXPAND, REMOVE_ANCHORS);
	printf("</a>");
	*item_is_open = True;
      } else {
	for (h = t->children; h != NULL; h = h->sister) toc(h, curlevel, item_is_open);
      }
      break;
    case Root:
      for (h = t->children; h != NULL; h = h->sister) toc(h, curlevel, item_is_open);
      break;
    default: assert(! "Cannot happen");
  }
}

/* expand -- write the tree, inserting ID's at H* and inserting a toc */
static void expand(Tree t, Boolean *write, Boolean exp, Boolean keep_anchors)
{
  Tree h;
  pairlist a;
  int level;
  Boolean item_is_open = False;

  for (h = t->children; h != NULL; h = h->sister) {
    switch (h->tp) {
      case Text:
	if (*write) printf("%s", h->text);
	break;
      case Comment:
	if (exp && (eq(h->text, TOC) || eq(h->text, BEGIN_TOC))) {
	  printf("<!--%s-->\n", BEGIN_TOC);
	  printf("<ul class=\"%s\">\n", TOC);
	  level = toc_low;
	  toc(get_root(t), &level, &item_is_open);
	  while (level > toc_low) {
	    printf(xml ? "</li>\n" : "\n");
	    indent(level - toc_low);
	    printf("</ul>");
	    level--;
	  }
	  if (item_is_open && xml) printf("</li>\n");
	  printf("</ul>\n");
	  printf("<!--%s-->", END_TOC);
	  if (eq(h->text, BEGIN_TOC))
	    *write = False;			/* Suppress old ToC */
	} else if (exp && eq(h->text, END_TOC)) {
	  *write = True;
	} else {
	  printf("<!--%s-->", h->text);
	}
	break;
      case Declaration:
	printf("<!DOCTYPE %s", h->name);
	if (h->text) printf(" PUBLIC \"%s\"", h->text);	else printf(" SYSTEM");
	if (h->url) printf(" \"%s\"", h->url);
	printf(">");
	break;
      case Procins:
	if (*write) printf("<?%s>", h->text);
	break;
      case Element:
	if (eq(h->name, "h1") || eq(h->name, "h2") || eq(h->name, "h3")
	    || eq(h->name, "h4") || eq(h->name, "h5") || eq(h->name, "h6")) {
	  /* Give headers an ID, if they don't have one */
	  if (! get_attrib(h, "id", NULL)) set_attrib(h, "id", gen_id(h));
	}
	if (*write) {
	  if (! keep_anchors && eq(h->name, "a")) {
	    /* Don't write the <a> and </a> tags */
	    expand(h, write, exp, False);
	  } else if (! keep_anchors && eq(h->name, "span")
		     && has_class(h->attribs, INDEX)) {
	    /* Don't write <span.index>...</span> tags */
	    expand(h, write, exp, False);
	  } else if (eq(h->name, "a") && (has_class(h->attribs, TARGET)
		     || has_class(h->attribs, TOC))) {
	    /* This <a> was inserted by toc itself; remove it */
	    expand(h, write, exp, False);
	  } else {
	    printf("<%s", h->name);
	    for (a = h->attribs; a != NULL; a = a->next) {
	      if (keep_anchors || !eq(a->name, "id")) {
		/* If we don't keep anchors, we don't keep IDs either */
		printf(" %s", a->name);
		if (a->value != NULL) printf("=\"%s\"", a->value);
	      }
	    }
	    if (is_empty(h->name)) {
	      assert(h->children == NULL);
	      printf(xml ? " />" : ">");
	    } else {
	      string val;
	      printf(">");
	      /* Insert an <A NAME> if element has an ID and is not <A> */
	      if (bctarget && is_mixed(h->name) && get_attrib(h, "id", &val)
		  && !eq(h->name, "a") && ! xml)
		printf("<a class=\"%s\" name=\"%s\"></a>", TARGET, val);
	      expand(h, write, exp, keep_anchors);
	      printf("</%s>", h->name);
	    }
	  }
	}
	break;
      case Root:
	assert(! "Cannot happen");
	break;
      default:
	assert(! "Cannot happen");
    }
  }
}

/* usage -- print usage message and exit */
static void usage(string name)
{
  errexit("Version %s\nUsage: %s [-l low] [-h high] [-x] [-t] [html-file]\n",
	  VERSION, name);
}


int main(int argc, char *argv[])
{
  int i;
  Boolean write = True;

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

  yyin = stdin;
  for (i = 1; i < argc; i++) {
    if (eq(argv[i], "-l")) {
      if (i >= argc - 1) usage(argv[0]);
      toc_low = atoi(argv[++i]);
    } else if (eq(argv[i], "-h")) {
      if (i >= argc - 1) usage(argv[0]);
      toc_high = atoi(argv[++i]);
    } else if (eq(argv[i], "-x")) {
      xml = True;
    } else if (eq(argv[i], "-t")) {
      bctarget = False;
    } else if (eq(argv[i], "-")) {
      /* yyin = stdin; */
    } else {
      yyin = fopenurl(argv[i], "r");
      if (yyin == NULL) {
	perror(argv[1]);
	exit(2);
      }
    }
  }
  if (toc_low < 1) toc_low = 1;
  if (toc_high > 6) toc_high = 6;

  if (yyparse() != 0) exit(3);

  tree = get_root(tree);
  expand(tree, &write, EXPAND, KEEP_ANCHORS);
  tree_delete(tree);				/* Just to test memory mgmt */
  return 0;
}
