/*
 * Insert an index between "<!--begin-index-->" and "<!--end-index-->",
 * or replacing the comment "<!--index-->"
 *
 * The index links to elements with ID attributes as well as with
 * empty <A NAME> elements.
 *
 * Any <A> tags with a class of "bctarget" are not copied, but
 * regenerated. They are assumed to be backwards-compatible versions
 * of ID attributes on their parent elements. But if the option -t or
 * -x are given, those <A> elements are removed.
 *
 * There's a limit of 100000 index terms (10^(MAXIDLEN-1)).
 *
 * Index terms are elements with a class of "index", "index-inst" or
 * "index-def", as well as all <dfn> elements. The contents of the
 * element is the index term, unless the element has a title
 * attribute. The title attribute can contain "|" and "!!":
 *
 * "term"
 * "term1|term2|term3|..."
 * "term!!subterm!!subsubterm!!..."
 * "term1!!subterm1|term2!!subterm2|..."
 * etc.
 *
 * For backward compatibility with an earlier Perl program, "::" is
 * accepted as an alternative for "!!", but it is better not to use
 * both separators in the same project, since the sorting maybe
 * adversely affected.
 *
 * Class "index-def" results in a bold entry in the index, "index" in
 * a normal one. "index-inst" is an alias for "index", provided for
 * backward compatibility.
 *
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 11 Apr 2000
 * Version: $Id: index.c,v 1.16 2003/04/09 10:07:12 bbos Exp $
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
#else
extern int errno;
char *strerror(int errnum);
int strerror_r(int errnum, char *buf, size_t n);
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
#include "genid.e"
#include "errexit.e"
#include "class.e"


#define BEGIN_INDEX "begin-index" /* <!--begin-index--> */
#define END_INDEX "end-index"	/* <!--end-index--> */
#define INDEX "index"		/* <!--index--> */
#define INDEX_INST "index-inst"	/* class="index-inst" */
#define INDEX_DEF "index-def"	/* class="index-def" */
#define TARGET "bctarget"	/* CLASS="...bctarget..." */

#define MAXSTR 2048		/* Max. length of URL + term */
#define MAXSUBS 20		/* Max. depth of subterms */

typedef struct _indexterm {
  string term, url;
  int importance;		/* 1 (low) or 2 (high) */
} *Indexterm;

static Tree tree;
static Boolean xml = False;	/* Use <empty /> convention */
static string base = NULL;	/* (Rel.) URL of output file */
static string indexdb = NULL;	/* Persistent store of terms */
static FILE *globalfile;	/* Must be global for twalk */
static string globalprevious;	/* Must be global for twalk */
static Boolean bctarget = True;	/* Add <A name=> after IDs */


/* handle_error -- called when a parse error occurred */
EXPORT void handle_error(void *clientdata, const string s, int lineno)
{
  (void) fprintf(stderr, "%d: %s\n", lineno, s);
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
  tree = html_push(tree, name, attribs);
}

/* handle_endtag -- called after an endtag is parsed (name may be "") */
EXPORT void handle_endtag(void *clientdata, string name)
{
  tree = html_pop(tree, name);
}

/* indent -- print newline and n times 2 spaces */
static void indent(int n)
{
  putchar('\n');
  for (; n > 0; n--) printf("  ");
}

/* write_index_item -- write one item in the list of index terms */
static void write_index_item(const void *term1, const VISIT which,
			     const int depth)
{
  string sub[MAXSUBS], oldsub[MAXSUBS];
  string p;
  Indexterm term = *(Indexterm*)term1;
  int i, j, n, oldn;

  if (which != postorder && which != leaf) return;

  p = term->term;
  n = 0;
  while (p) {
    sub[n] = p;
    p = strstr(sub[n], "!!");
    if (!p) p = strstr(sub[n], "::");		/* Backwards compatibility */
    if (p) p += 2;
    n++;
  }
  sub[n] = sub[n-1] + strlen(sub[n-1]) + 2;

  p = globalprevious;
  oldn = 0;
  while (p) {
    oldsub[oldn] = p;
    p = strstr(oldsub[oldn], "!!");
    if (!p) p = strstr(oldsub[oldn], "::");	/* Backwards compatibility */
    if (p) p += 2;
    oldn++;
  }
  oldsub[oldn] = oldsub[oldn-1] + strlen(oldsub[oldn-1]) + 2;

  /* Count how many subterms are equal to the previous entry */
  for (i = 0; i < min(n, oldn)
	 && sub[i+1] - sub[i] == oldsub[i+1] - oldsub[i]
	 && strncasecmp(sub[i], oldsub[i], sub[i+1] - sub[i] - 2) == 0;
       i++) ;

  for (j = oldn - 1; j > i; j--) {indent(j); printf("</ul>");}
  if (n > oldn && oldn == i) {indent(i); printf("<ul>");}

  /* Print new (sub)terms */
  for (j = i; j < n; j++) {
    indent(j); printf("<li>");
    for (p = sub[j]; p != sub[j+1] - 2; p++) putchar(*p);
    if (j != n - 1) {indent(j+1); printf("<ul>");}
  }

  /* Print a link */
  switch (term->importance) {
    case 1: printf(", <a href=\"%s\">#</a>", term->url); break;
    case 2: printf(", <a href=\"%s\"><strong>#</strong></a>",term->url); break;
    default: assert(! "Cannot happen\n");
  }

  /* Remember this term */
  globalprevious = term->term;
}

/* mkindex -- write out an index */
static void mkindex(Indexterm terms)
{
  string p, h;

  printf("<ul class=\"indexlist\">");
  globalprevious = "";
  twalk(terms, write_index_item);

  /* Close all open lists */
  p = globalprevious;
  while (p) {
    printf("\n</ul>");
    h = p;
    p = strstr(h, "!!");
    if (!p) p = strstr(h, "::");		/* Backwards compatibility */
    if (p) p += 2;
  }
}

/* expand -- write the tree, add <A NAME> if needed and replace <!--index--> */
static void expand(Tree t, Boolean *write, Indexterm terms)
{
  Tree h;
  pairlist a;
  string val;
  Boolean do_tag;

  for (h = t->children; h != NULL; h = h->sister) {
    switch (h->tp) {
      case Text:
	if (*write) printf("%s", h->text);
	break;
      case Comment:
	/* To do: trim whitespace first */
	if (eq(h->text, INDEX) || eq(h->text, BEGIN_INDEX)) {
	  printf("<!--%s-->\n", BEGIN_INDEX);
	  mkindex(terms);
	  printf("<!--%s-->", END_INDEX);
	  if (eq(h->text, BEGIN_INDEX)) *write = False;	/* Skip old index */
	} else if (eq(h->text, END_INDEX)) {
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
	if (*write) {
	  /* If an <a> was inserted by index itself, remove it */
	  do_tag = !eq(h->name, "a") || !has_class(h->attribs, TARGET);
	  if (do_tag) {
	    printf("<%s", h->name);
	    for (a = h->attribs; a != NULL; a = a->next) {
	      printf(" %s", a->name);
	      if (a->value != NULL) printf("=\"%s\"", a->value);
	    }
	    assert(! is_empty(h->name) || h->children == NULL);
	    printf(xml && is_empty(h->name) ? " />" : ">");
	    /* Insert an <A NAME> if element has an ID and is not <A> */
	    if (bctarget && is_mixed(h->name) && get_attrib(h, "id", &val)
		&& !eq(h->name, "a") && ! xml)
	      printf("<a class=\"%s\" name=\"%s\"></a>", TARGET, val);
	  }
	  expand(h, write, terms);
	  if (do_tag && ! is_empty(h->name)) printf("</%s>", h->name);
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

/* termcmp -- comparison routine for Indexterms */
static int termcmp(const void *a1, const void *b1)
{
  Indexterm a = (Indexterm)a1, b = (Indexterm)b1;
  string p, q;

  assert(a);
  assert(b);
  assert(a->term);
  assert(b->term);
  assert(a->url);
  assert(b->url);

  for (p = a->term, q = b->term;; p++, q++) {
    while (*p && !isalnum(*p)) p++;		/* Skip punctuation */
    while (*q && !isalnum(*q)) q++;		/* Skip punctuation */
    if (tolower(*p) < tolower(*q)) return -1;	/* a before b */
    if (tolower(*p) > tolower(*q)) return 1;	/* b before a */
    if (!*p) break;				/* Same up till the end */
  }

  /* Terms are the same; sort on URL instead */
  return strcmp(a->url, b->url);
}

/* copy_contents -- recursively expand contents of element t into a string */
static void copy_contents(Tree t, string *s)
{
  Tree h;
  int i;
  pairlist a;
  string p;

  for (h = t->children; h != NULL; h = h->sister) {
    switch (h->tp) {
      case Text:
	i = *s ? strlen(*s) : 0;
	renewarray(*s, i + strlen(h->text) + 1);
	/* Copy, but transform all whitespace to spaces */
	for (p = h->text; *p; p++, i++) (*s)[i] = isspace(*p) ? ' ' : *p;
	(*s)[i] = '\0';
	break;
      case Comment: break;
      case Declaration: break;
      case Procins: break;
      case Element:
	/* Only certain tags are retained */
	if (eq(h->name, "span") || eq(h->name, "code") || eq(h->name, "tt")
	    || eq(h->name, "acronym") || eq(h->name, "abbr")
	    || eq(h->name, "bdo") || eq(h->name, "kbd") || eq(h->name, "samp")
	    || eq(h->name, "sub") || eq(h->name, "sup")
	    || eq(h->name, "var")) {
	  strapp(s, "<", h->name, NULL);
	  for (a = h->attribs; a != NULL; a = a->next) {
	    if (! a->value) strapp(s, " ", a->name, NULL);
	    else strapp(s, " ", a->name, "=\"", a->value, "\"", NULL);
	  }
	  assert(! is_empty(h->name) || h->children == NULL);
	  if (is_empty(h->name)) {
	    strapp(s, xml ? " />" : ">", NULL);
	  } else {
	    strapp(s, ">", NULL);
	    copy_contents(h, s);
	    strapp(s, "</", h->name, ">", NULL);
	  }
	} else {				/* Ignore tag, copy contents */
	  copy_contents(h, s);
	}
	break;
      case Root: assert(! "Cannot happen"); break;
      default: assert(! "Cannot happen");
    }
  }
}

/* copy_to_index -- copy the contents of element h to the index db */
static void copy_to_index(Tree t, Indexterm *terms, int importance)
{
  string id, title;
  Indexterm term;
  int i, n;

  assert(get_attrib(t, "id", NULL));
  get_attrib(t, "id", &id);

  if (get_attrib(t, "title", &title)) {		/* Parse title, not contents */

    i = 0;
    while (title[i]) {
      n = strcspn(title + i, "|");		/* Find | or \0 */
      new(term);
      term->importance = importance;
      term->url = NULL;
      strapp(&term->url, base, "#", id, NULL);
      term->term = newnstring(title + i, n);
      if (! tsearch(term, (void**)terms, termcmp))
	errexit("Out of memory while parsing term %s\n", term->term);
      i += n;
      if (title[i]) i++;			/* Skip '|' */
    }

  } else {					/* Recursively copy contents */

    new(term);
    term->importance = importance;
    term->url = term->term = NULL;
    strapp(&term->url, base, "#", id, NULL);
    copy_contents(t, &term->term);
    if (term->term)				/* Non-empty contents */
      if (! tsearch(term, (void**)terms, termcmp))
	errexit("Out of memory while parsing term %s\n", term->term);

  }
}

/* collect -- collect index terms, add IDs where needed */
static void collect(Tree t, Indexterm *terms)
{
  int importance;
  Tree h;

  for (h = t->children; h != NULL; h = h->sister) {
    switch (h->tp) {
      case Text: case Comment: case Declaration: case Procins: break;
      case Element:
	if (eq(h->name, "dfn")) importance = 2;
	else if (has_class(h->attribs,INDEX)||has_class(h->attribs,INDEX_INST))
	  importance = 1;
	else if (has_class(h->attribs, INDEX_DEF)) importance = 2;
	else importance = 0;
	if (importance != 0) {
	  /* Give it an ID, if it doesn't have one */
	  if (! get_attrib(h, "id", NULL)) set_attrib(h, "id", gen_id(h));
	  copy_to_index(h, terms, importance);
	} else {
	  collect(h, terms);
	}
	break;
      case Root: assert(! "Cannot happen"); break;
      default: assert(! "Cannot happen");
    }
  }
}

/* load_index -- read persistent term db from file */
static void load_index(const string indexdb, Indexterm *terms)
{
  FILE *f;
  int n;
  char line[MAXSTR];
  Indexterm term;

  if (! (f = fopen(indexdb, "r"))) return;	/* Assume file not found... */

  while (fgets(line, sizeof(line), f)) {
    chomp(line);
    n = strcspn(line, "\t");
    if (line[n] != '\t') errexit("Illegal syntax in %s\n", indexdb);
    new(term);
    term->term = newnstring(line, n);
    switch (line[n + 1]) {
      case '1': term->importance = 1; break;
      case '2': term->importance = 2; break;
      default: errexit("Error in %s (column 2 must be '1' or '2')\n", indexdb);
    }
    if (line[n+2] != '\t') errexit("Illegal syntax in %s\n", indexdb);
    term->url = newstring(line + n + 3);
    if (! tsearch(term, (void**)terms, termcmp))
      errexit("Out of memory while loading %s\n", indexdb);
  }

  fclose(f);
}

/* save_a_term -- write one term to globalfile */
static void save_a_term(const void *term1, const VISIT which, const int dp)
{
  Indexterm term = *(Indexterm*)term1;

  if (which == endorder || which == leaf)
    fprintf(globalfile, "%s\t%d\t%s\n", term->term,term->importance,term->url);
}

/* save_index -- write terms to file */
static void save_index(const string indexdb, Indexterm terms)
{
  if (! (globalfile = fopen(indexdb, "w")))
    errexit("%s: %s\n", indexdb, strerror(errno));
  twalk(terms, save_a_term);
  fclose(globalfile);
}

/* usage -- print usage message and exit */
static void usage(string name)
{
  errexit("Version %s\nUsage: %s [-i indexdb] [-b base] [-x] [-t] [html-file]\n",
	  VERSION, name);
}


int main(int argc, char *argv[])
{
  int i;
  Boolean write = True;
  Indexterm termtree = NULL;			/* Sorted tree of terms */

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

  yyin = NULL;
  for (i = 1; i < argc && argv[i][0] == '-' && !eq(argv[i], "--"); i++) {
    switch (argv[i][1]) {
      case 't':					/* Don't write <a name> after each ID */
	bctarget = False;
	break;
      case 'x':					/* Output as XML */
	xml = True;
	break;
      case 'b':					/* Set base of URL */
	base = strdup(argv[i][2] ? argv[i] + 2 : argv[++i]);
	break;
      case 'i':					/* Set name of index db */
	indexdb = strdup(argv[i][2] ? argv[i] + 2 : argv[++i]);
	break;
      case '\0':
	yyin = stdin;
	break;
      default:
	usage(argv[0]);
    }
  }
  if (i < argc && eq(argv[i], "--")) i++;

  if (i == argc) {if (!yyin) yyin = stdin;}
  else if (i == argc - 1 && eq(argv[i], "-")) yyin = stdin;
  else if (i == argc - 1) yyin = fopenurl(argv[i], "r");
  else usage(argv[0]);

  if (yyin == NULL) {perror(argv[i]); exit(1);}

  if (!base && i == argc - 1) base = newstring(argv[i]);
  if (!base) base = newstring("");

  /* Read the index DB into memory */
  if (indexdb) load_index(indexdb, &termtree);

  /* Parse, build tree, collect existing IDs */
  if (yyparse() != 0) exit(3);

  /* Scan for index terms, add them to the tree, add IDs where needed */
  collect(get_root(tree), &termtree);

  /* Write out the document, adding <A NAME> and replacing <!--index--> */
  expand(get_root(tree), &write, termtree);

  /* Store terms to file */
  if (indexdb) save_index(indexdb, termtree);

  fclose(yyin);
#if 0
  tree_delete(tree);				/* Just to test memory mgmt */
#endif
  return 0;
}
