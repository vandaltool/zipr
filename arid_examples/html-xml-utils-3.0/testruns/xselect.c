/*
 * select -- extract elements matching a selector
 *
 * Assumes that class selectors (".foo") refer to an attribute called
 * "class".
 *
 * Assumes that ID selectors ("#foo") refer to an attribute called
 * "id".
 *
 * Options:
 *
 * -l language
 *
 *     Sets the default language, in case the root element doesn't
 *     have an xml: lang attribute. Example: -l en. Default: none.
 *
 * -s separator
 *
 *     A string to print after each match. Accepts C-like escapes.
 *     Example: -s '\n\n' to print an empty line after each match.
 *     Default: empty string.
 *
 * -i
 *
 *     Match case-insensitively. Useful for HTML and some other
 *     SGML-based languages.
 *
 * -c
 *
 *     Print content only. Without -c, the start and end tag of the
 *     matched element are printed as well; with -c only the contents
 *     of the matched element are printed.
 *
 * Copyright © 2001 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 5 Jul 2001
 * Version: $Id: xselect.c,v 1.7 2003/04/09 10:07:13 bbos Exp $
 *
 **/
#include <config.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
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
#include "export.h"
#include "types.e"
#include "heap.e"
#include "html.e"
#include "scan.e"
#include "errexit.e"
#include "selector.e"


typedef struct _Node {
  string name;
  pairlist attribs;
  string language;
  struct _Node *child;				/* Youngest(!) child */
  struct _Node *sister;				/* Older(!) sister */
  struct _Node *parent;
} Node, *Tree;

static Tree tree = NULL;			/* Current elt in tree */
static Selector selector;			/* The selector to match */
static int copying = 0;				/* >0 means: inside a match */
static string language = "";			/* Initial language */
static Boolean content_only = False;		/* Omit start/end tag */
static string separator = "";			/* Printed between matches */
static Boolean case_insensitive = False;	/* How to match elems/attrs */


/* push -- add an element to the tree, make tree point to that element */
static void push(const string name, const pairlist attribs)
{
  pairlist p;
  Node *h;

  new(h);
  h->name = name;
  h->attribs = attribs;
  for (p = attribs; p && !eq(p->name, "xml:lang"); p = p->next) ;
  if (p) h->language = p->value;		/* Explicit */
  else if (tree) h->language = tree->language;	/* Inherit */
  else h->language = language;			/* Initial */
  h->child = NULL;
  h->parent = tree;
  h->sister = tree ? tree->child : NULL;
  if (tree) tree->child = h;
  tree = h;
}

/* pop -- make tree point to parent of current node */
static void pop(const string name)
{
  if (!tree || !tree->name || !eq(tree->name, name))
    errexit("Input is not well-formed. (Maybe try normalize?)\n");
  tree = tree->parent;
}

/* same -- compare two names, case-(in)sensitively, depending */
static Boolean same(const string a, const string b)
{
  return case_insensitive ? strcasecmp(a, b) == 0 : eq(a, b);
}

/* nr_sisters -- return # of elder sisters of a node */
static int nr_sisters(const Node *n)
{
  if (!n->sister) return 0;
  else return 1 + nr_sisters(n->sister);
}

/* nr_typed_sisters -- return # of elder sisters of type t */
static int nr_typed_sisters(const Node *n, const Node *t)
{
  if (!n->sister)
    return 0;
  else if (!same(n->sister->name, t->name))
    return nr_typed_sisters(n->sister, t);
  else
    return 1 + nr_typed_sisters(n->sister, t);
}

/* get_attr -- return the value of the named attribute, or NULL */
static string get_attr(const Node *n, const string name)
{
  pairlist p;

  for (p = n->attribs; p && !same(p->name, name); p = p->next) ;
  return p ? p->value : NULL;
}

/* includes -- check for word in the space-separated words of line */
static Boolean includes(const string line, const string word)
{
  int i = 0, n = strlen(word);

  /* What should happen if word is the empty string? */
  /* To do: compare with contains() in class.c, keep the best */
  while (line[i]) {
    if (case_insensitive) {
      if (!strncasecmp(line+i, word, n) && (!line[i+n] || isspace(line[i+n])))
	return True;
    } else {
      if (!strncmp(line+i, word, n) && (!line[i+n] || isspace(line[i+n])))
	return True;
    }
    do i++; while (line[i] && !isspace(line[i]));
    while (isspace(line[i])) i++;
  }
  return False;
}

/* starts_with -- check if line starts with prefix */
static Boolean starts_with(const string line, const string prefix)
{
  return case_insensitive
    ? strncasecmp(line, prefix, strlen(prefix)) == 0
    : strncmp(line, prefix, strlen(prefix)) == 0;
}

/* ends_with -- check if line ends with suffix */
static Boolean ends_with(const string line, const string suffix)
{
  int n1 = strlen(line), n2 = strlen(suffix);
  return n1 >= n2 && eq(line + n1 - n2, suffix);
}

/* contains -- check if line contains s */
static Boolean contains(const string line, const string s)
{
  return strstr(line, s) != NULL;
}

/* lang_match -- check if language specific is subset of general */
static Boolean lang_match(const string specific, const string general)
{
  int n = strlen(general);
  return !strncasecmp(specific, general, n)
    && (specific[n] == '-' || !specific[n]);
}

/* simple_match -- check if a node matches a simple selector */
static Boolean simple_match(const Node *n, const SimpleSelector *s)
{
  AttribCond *p;
  PseudoCond *q;
  string h;
  int i;

  /* Match the type selector */
  if (s->name && !same(s->name, n->name)) return False;

  /* Match the attribute selectors, including class and ID */
  for (p = s->attribs; p; p = p->next) {
    if (!(h = get_attr(n, (p->op == HasClass) ? (string)"class"
		       : (p->op == HasID) ? (string) "id" : p->name)))
      return False;
    switch (p->op) {
    case Exists: break;
    case Equals:
    case HasID: if (!eq(p->value, h)) return False; break;
    case Includes:
    case HasClass: if (!includes(h, p->value)) return False; break;
    case StartsWith: if (!starts_with(h, p->value)) return False; break;
    case EndsWidth: if (!ends_with(h, p->value)) return False; break;
    case Contains: if (!contains(h, p->value)) return False; break;
    case LangMatch: if (!lang_match(h, p->value)) return False; break;
    default: assert(!"Cannot happen");
    }
  }

  /* Match the pseudo-classes */
  for (q = s->pseudos; q; q = q->next) {
    switch (q->type) {
    case Root:
      if (n->parent) return False;
      break;
    case NthChild:
      i = nr_sisters(n) + 1;
      if (q->a == 0) {if (i != q->b) return False;}
      else {if ((i-q->b)/q->a < 0 || (i-q->b)%q->a != 0) return False;}
      break;
    case NthOfType:
      i = nr_typed_sisters(n, n) + 1;
      if (q->a == 0) {if (i != q->b) return False;}
      else {if ((i-q->b)/q->a < 0 || (i-q->b)%q->a != 0) return False;}
      break;
    case FirstChild:
      if (n->sister) return False;
      break;
    case FirstOfType:
      if (nr_typed_sisters(n, n) != 0) return False;
      break;
    case Lang:
      if (!lang_match(n->language, q->s)) return False;
      break;
    default:
      assert(!"Cannot happen");
    }
  }
  return True;
}

/* matches_sel -- check if node matches selector (recursively) */
static Boolean matches_sel(const Tree t, const Selector s)
{
  Tree h;

  if (!simple_match(t, s)) return False;
  if (!s->context) return True;
  switch (s->combinator) {
  case Descendant:
    for (h = t->parent; h && !matches_sel(h, s->context); h = h->parent);
    return h != NULL;
  case Child:
    return t->parent && matches_sel(t->parent, s->context);
  case Adjacent:
    return t->sister && matches_sel(t->sister, s->context);
  case Sibling:
    for (h = t->sister; h && !matches_sel(h, s->context); h = h->sister);
    return h != NULL;
  default:
    assert(!"Cannot happen");
    return False;
  }
}

/* matches -- check if current node matches the selector */
static Boolean matches(void)
{
  return matches_sel(tree, selector);
}

/* printtag -- print a start tag or an XML-style empty tag */
static void printtag(const string name, const pairlist attribs,
		     Boolean slash)
{
  pairlist p;

  printf("<%s", name);
  for (p = attribs; p; p = p->next) {
    printf(" %s=\"%s\"", p->name, p->value);
  }
  if (slash) putchar('/');
  putchar('>');
}

/* printsep -- print the separator string, interpret escapes */
static void printsep(const string separator)
{
  string s = separator;
  int c;

  while (*s) {
    if (*s != '\\') putchar(*(s++));
    else if ('0' <= *(++s) && *s <= '7') {
      c = *s - '0';
      if ('0' <= *(++s) && *s <= '7') {
	c = 8 * c + *s - '0';
	if ('0' <= *(++s) && *s <= '7') c = 8 * c + *s - '0';
      }
      putchar(c); s++;
    } else
      switch (*s) {
      case '\0': putchar('\\'); break;
      case 'n': putchar('\n'); s++; break;
      case 't': putchar('\t'); s++; break;
      case 'r': putchar('\r'); s++; break;
      case 'f': putchar('\f'); s++; break;
      default: putchar(*(s++)); break;
      }
  }
}


/* handle_error -- called when a parse error occurred */
static void handle_error(void *clientdata, const string s, int lineno)
{
  fprintf(stderr, "%d: %s\n", lineno, s);
}

/* start -- called before the first event is reported */
static void* start(void)
{
  return NULL;
}
  
/* end -- called after the last event is reported */
static void end(void *clientdata)
{
}

/* handle_comment -- called after a comment is parsed */
static void handle_comment(void *clientdata, const string commenttext)
{
  free(commenttext);
}

/* handle_text -- called after a text chunk is parsed */
static void handle_text(void *clientdata, const string text)
{
  if (copying) printf("%s", text);
  free(text);
}

/* handle_declaration -- called after a declaration is parsed */
static void handle_decl(void *clientdata, const string gi, const string fpi,
			const string url)
{
  free(gi);
  free(fpi);
  free(url);
}

/* handle_proc_instr -- called after a PI is parsed */
static void handle_pi(void *clientdata, const string pi_text)
{
  free(pi_text);
}

/* handle_starttag -- called after a start tag is parsed */
static void handle_starttag(void *clientdata, const string name,
			    pairlist attribs)
{
  assert(copying >= 0);
  push(name, attribs);				/* Add to tree */
  if (copying || matches()) copying++;		/* Level of copying */
  if (copying > 1 || (copying == 1 && !content_only))
    printtag(name, attribs, False);		/* Print a start tag */
}

/* handle_emptytag -- called after an empty tag is parsed */
static void handle_emptytag(void *clientdata, const string name,
			    pairlist attribs)
{
  assert(copying >= 0);
  push(name, attribs);				/* Add to tree */
  if (copying || matches()) copying++;		/* Level of copying */
  if (copying > 1 || (copying == 1 && !content_only))
    printtag(name, attribs, True);		/* Print a start tag */
  if (copying == 1) printsep(separator);	/* Separate the matches */
  if (copying) copying--;
  pop(name);					/* Remove from tree again */
}

/* handle_endtag -- called after an endtag is parsed (name may be "") */
static void handle_endtag(void *clientdata, const string name)
{
  assert(copying >= 0);
  if (copying > 1 || (copying == 1 && !content_only)) printf("</%s>", name);
  if (copying == 1) printsep(separator);	/* Separate the matches */
  if (copying) copying--;
  pop(name);
  free(name);
}

/* usage -- print usage message and exit */
static void usage(const string name)
{
  errexit("Version %s\n\
Usage: %s [-i] [-c] [-l language] [-s separator] selector\n", VERSION, name);
}


int main(int argc, char *argv[])
{
  string s;
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

  /* Command line options */
  while ((c = getopt(argc, argv, "icl:s:")) != -1) {
    switch (c) {
    case 'c': content_only = True; break;
    case 'l': language = optarg; break;
    case 's': separator = optarg; break;
    case 'i': case_insensitive = True;  break;
    case '?': usage(argv[0]); break;
    default: assert(!"Cannot happen");
    }
  }

  /* Parse the selector */
  if (optind >= argc) usage(argv[0]);		/* Need at least 1 arg */
  for (s = newstring(argv[optind++]); optind < argc; optind++)
    strapp(&s, " ", argv[optind], NULL);
  selector = parse_selector(s);

#if 0
  /* Debugging */
  fprintf(stderr, "--> %s\n", selector_to_string(selector));
#endif

  /* Walk the tree */
  yyin = stdin;
  if (yyparse() != 0) exit(3);
  return 0;
}
