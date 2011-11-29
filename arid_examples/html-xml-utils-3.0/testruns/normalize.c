/*
 * Format an HTML source in a consistent manner.
 *
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Created 9 May 1998
 * Bert Bos <bert@w3.org>
 * $Id: normalize.c,v 1.23 2003/12/02 18:19:24 bbos Exp $
 */
#include <config.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#ifdef HAVE_STRING_H
#  include <string.h>
#elif HAVE_STRINGS_H
#  include <strings.h>
#endif
#include <assert.h>
#include "export.h"
#include "types.e"
#include "tree.e"
#include "html.e"
#include "scan.e"
#include "textwrap.e"
#include "openurl.e"

static Tree tree;
static Boolean do_xml = False;
static Boolean do_endtag = False;
static Boolean has_errors = False;
static Boolean do_doctype = True;


/* handle_error -- called when a parse error occurred */
void handle_error(void *clientdata, const string s, int lineno)
{
  fprintf(stderr, "%d: %s\n", lineno, s);
  has_errors = True;
}

/* start -- called before the first event is reported */
void* start(void)
{
  tree = create();
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
  tree = append_comment(tree, commenttext);
}

/* handle_text -- called after a text chunk is parsed */
void handle_text(void *clientdata, string text)
{
  tree = append_text(tree, text);
}

/* handle_decl -- called after a declaration is parsed */
void handle_decl(void *clientdata, string gi,
		 string fpi, string url)
{
  tree = append_declaration(tree, gi, fpi, url);
}

/* handle_pi -- called after a PI is parsed */
void handle_pi(void *clientdata, string pi_text)
{
  tree = append_procins(tree, pi_text);
}

/* handle_starttag -- called after a start tag is parsed */
void handle_starttag(void *clientdata, string name, pairlist attribs)
{
  tree = html_push(tree, name, attribs);
}

/* handle_emptytag -- called after an empty tag is parsed */
void handle_emptytag(void *clientdata, string name, pairlist attribs)
{
  tree = html_push(tree, name, attribs);
}

/* handle_endtag -- called after an endtag is parsed (name may be "") */
void handle_endtag(void *clientdata, string name)
{
  tree = html_pop(tree, name);
  free(name);
}

/* insert -- insert an attribute into a sorted list of attributes */
static pairlist insert(pairlist x, pairlist list)
{
  if (! list) {					/* Empty list */
    x->next = NULL;
    return x;
  } else if (strcmp(x->name, list->name) <= 0) { /* Insert at head */
    x->next = list;
    return x;
  } else {					/* Insert not at head */
    list->next = insert(x, list->next);
    return list;
  }
}

/* sort_list -- sort a linked list of attributes, return reordered list */
static pairlist sort_list(pairlist list)
{
  /* Insertion sort should be fast enough... */
  if (! list) return NULL;
  else return insert(list, sort_list(list->next));
}

/* only_space -- check if s contains only whitespace */
static Boolean only_space(string s)
{
  while (*s == ' ' || *s == '\n' || *s == '\r' || *s == '\t' || *s == '\f')
    s++;
  return *s == '\0';
}

/* pp -- print the document normalized */
static void pp(Tree n, Boolean preformatted, Boolean allow_text)
{
  Boolean pre, mixed;
  string s;
  pairlist h;
  size_t i;
  Tree l;

  switch (n->tp) {
    case Text:
      if (!allow_text) {
	assert(only_space(n->text));
      } else {
	s = n->text;
	i = strlen(s);
#if 0
	if (s[0] == '\r' && s[1] == '\n') {s += 2; i -= 2;}
	else if (s[0] == '\n' || s[0] == '\r') {s++; i--;}
	if (i > 1 && s[i-1] == '\n' && s[i-2] == '\r') i -= 2;
	else if (i > 0 && (s[i-1] == '\r' || s[i-1] == '\n')) i--;
#endif
	outn(s, i, preformatted);
      }
      break;
    case Comment:
      out("<!--", True); out(n->text, True);
      if (allow_text || preformatted) out("-->", True);
      else outln("-->", preformatted);
      break;
    case Declaration:
      if (do_doctype) {
	out("<!DOCTYPE ", False);
	out(n->name, False);
	if (n->text) {
	  out(" PUBLIC \"", False);
	  out(n->text, False);
	  out("\"", False);
	} else
	  out(" SYSTEM", False);
	if (n->url) {
	  out(" \"", False);
	  out(n->url, False);
	  out("\"", False);
	}
	outln(">", False);
      }
      break;
    case Procins:
      out("<?", False); out(n->text, True);
      if (allow_text || preformatted) out(">", False);
      else outln(">", False);
      break;
    case Element:
      if (!preformatted && break_before(n->name)) outln(NULL, False);
      out("<", preformatted); out(n->name, preformatted);
      if (break_before(n->name)) inc_indent();
      n->attribs = sort_list(n->attribs);
      for (h = n->attribs; h != NULL; h = h->next) {
	out(" ", preformatted); out(h->name, preformatted);
	if (h->value != NULL) {
	  out("=\"", preformatted);
	  out(h->value, preformatted);
	  outc('"', preformatted);
	} else if (do_xml) {
	  out("=\"", preformatted);
	  out(h->name, preformatted);
	  outc('"', preformatted);
	}
      }
      if (is_empty(n->name)) {
	assert(n->children == NULL);
	out(do_xml ? " />" : ">", True);
	if (break_before(n->name)) dec_indent();
	if (!preformatted && break_after(n->name)) outln(NULL, False);
      } else {
	out(">", preformatted);
	pre = preformatted || is_pre(n->name);
	mixed = is_mixed(n->name);
	for (l = n->children; l != NULL; l = l->sister)
	  pp(l, pre, mixed);
	if (break_before(n->name)) dec_indent();
	if (do_xml || do_endtag || need_etag(n->name)
	    /* If followed by text, rather than an elt, add an end tag: */
	    || (n->sister && n->sister->tp == Text)) {
	  out("</", preformatted); out(n->name, preformatted);
	  out(">", preformatted);
	}
	if (!preformatted && break_after(n->name)) outbreak();
      }
      break;
    default:
      assert(!"Cannot happen");
  }
}

/* prettyprint -- print the tree normalized */
static void prettyprint(Tree t)
{
  Tree h;
  assert(t->tp == Root);
  for (h = t->children; h != NULL; h = h->sister) pp(h, False, False);
  flush();
}

/* usage -- print usage message and exit */
static void usage(string prog)
{
  fprintf(stderr, "%s version %s\n\
Usage: %s [-e] [-d] [-x] [-i indent] [-l linelen] [file_or_url]\n",
	  prog, VERSION, prog);
  exit(1);
}

/* main -- main body */
int main(int argc, char *argv[])
{
  int c;

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

  while ((c = getopt(argc, argv, "edxi:l:")) != -1)
    switch (c) {
    case 'e': do_endtag = True; break;
    case 'x': do_xml = True; break;
    case 'd': do_doctype = False; break;
    case 'i': set_indent(atoi(optarg)); break;
    case 'l': set_linelen(atoi(optarg)); break;
    default: usage(argv[0]);
    }
  if (optind == argc) yyin = stdin;
  else if (optind == argc - 1) yyin = fopenurl(argv[optind], "r");
  else usage(argv[0]);
  if (yyin == NULL) {perror(argv[optind]); exit(2);}
  if (yyparse() != 0) {exit(3);}
  tree = get_root(tree);
  prettyprint(tree);
  return has_errors ? 1 : 0;
}
