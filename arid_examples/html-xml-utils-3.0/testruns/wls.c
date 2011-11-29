/*
 * List all links from the given document.
 *
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Bert Bos <bert@w3.org>
 * Created 31 July 1999
 * $Id: wls.c,v 1.12 2000/08/07 12:28:44 bbos Exp $
 */
#include <config.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#if STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
# ifndef HAVE_STRDUP
#  include "strdup.e"
# endif
#endif
#include "export.h"
#include "types.e"
#include "html.e"
#include "scan.e"
#include "openurl.e"
#include "url.e"

extern int yylineno;				/* From scan.l */

static Boolean has_error = False;
static string base = NULL;
static string self;
static enum {Short, Long, HTML, Tuple} format = Short;	/* Option -l -h -t */
static Boolean relative = False;		/* Option -r */


/* down -- convert a string to lowercase, return pointer to arg */
static inline string down(string s)
{
  string t;
  for (t = s; *t; t++) *t = tolower(*t);
  return s;
}


/* attval -- find a named attribute in the list, return ptr to string */
static string attval(pairlist attribs, const string name)
{
  for (; attribs; attribs = attribs->next)
    if (strcasecmp(attribs->name, name) == 0)
      return attribs->value;
  return NULL;
}


/* output -- print the link (lowercases rel argument) */
static void output(string type, string rel, string url)
{
  string h = NULL;

  if (url) {					/* If we found a URL */
    if (! relative && base) {
      h = URL_s_absolutize(base, url);
      url = h;
    }
    if (rel) down(rel);
    switch (format) {
      case HTML:
	printf("<li><a class=\"%s\" rel=\"%s\" href=\"%s\">%s</a></li>\n",
		 type, rel ? rel : (string)"", url, url);
	break;
      case Long:
	printf("%s\t%s\t%s\n", type, rel ? rel : (string)"", url);
	break;
      case Short:
	printf("%s\n", url);
	break;
      case Tuple:
	printf("%s\t%s\t%s\t%s\n", self, type, rel ? rel : (string)"", url);
	break;
      default:
	assert(!"Cannot happen!");
    }
  }
  if (h) free(h);
}


/* --------------- implements parser interface api------------------------- */

/* handle_error -- called when a parse error occurred */
void handle_error(void *clientdata, const string s, int lineno)
{
  fprintf(stderr, "%d: %s\n", lineno, s);
  has_error = True;
}

/* start -- called before the first event is reported */
void* start(void)
{
  if (format == HTML) {
    printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\"\n");
    printf("  \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n");
    printf("<html>\n");
    printf("<head><title>Output of listlinks</title></head>\n");
    printf("<body>\n");
    printf("<ol>\n");
  }
  return NULL;
}
  
/* end -- called after the last event is reported */
void end(void *clientdata)
{
  if (format == HTML) {
    printf("</ol>\n");
    printf("</body>\n");
    printf("</html>\n");
  }
}

/* handle_comment -- called after a comment is parsed */
void handle_comment(void *clientdata, string commenttext)
{
  free(commenttext);
}

/* handle_text -- called after a text chunk is parsed */
void handle_text(void *clientdata, string text)
{
  /* There may be several consecutive calls to this routine. */
  /* escape(text); */
  free(text);
}

/* handle_decl -- called after a declaration is parsed */
void handle_decl(void *clientdata, string gi, string fpi, string url)
{
  /* skip */
  if (gi) free(gi);
  if (fpi) free(fpi);
  if (url) free(url);
}

/* handle_pi -- called after a PI is parsed */
void handle_pi(void *clientdata, string pi_text)
{
  if (pi_text) free(pi_text);
}

/* handle_starttag -- called after a start tag is parsed */
void handle_starttag(void *clientdata, string name, pairlist attribs)
{
  /* ToDo: print text of anchor, if available */
  string h;

  if (strcasecmp(name, "base") == 0) {
    h = attval(attribs, "href");
    if (h) base = strdup(h);			/* Use as base from now on */
    output("base", NULL, h);
  } else if (strcasecmp(name, "link") == 0) {
    output("link", attval(attribs, "rel"), attval(attribs, "href"));
  } else if (strcasecmp(name, "a") == 0) {
    output("a", attval(attribs, "rel"), attval(attribs, "href"));
  } else if (strcasecmp(name, "img") == 0) {
    output("img", NULL, attval(attribs, "src"));
    output("longdesc", NULL, attval(attribs, "longdesc"));
  } else if (strcasecmp(name, "input") == 0) {
    output("input", NULL, attval(attribs, "href"));
  } else if (strcasecmp(name, "object") == 0) {
    output("object", NULL,  attval(attribs, "data"));
  } else if (strcasecmp(name, "area") == 0) {
    output("area", attval(attribs, "rel"), attval(attribs, "href"));
  } else if (strcasecmp(name, "ins") == 0) {
    output("ins", NULL, attval(attribs, "cite"));
  } else if (strcasecmp(name, "del") == 0) {
    output("del", NULL, attval(attribs, "cite"));
  } else if (strcasecmp(name, "q") == 0) {
    output("q", NULL, attval(attribs, "cite"));
  } else if (strcasecmp(name, "blockquote") == 0) {
    output("bq", NULL, attval(attribs, "cite"));
  } else if (strcasecmp(name, "form") == 0) {
    output("form", attval(attribs, "method"), attval(attribs, "action"));
  } else if (strcasecmp(name, "frame") == 0) {
    output("frame", NULL, attval(attribs, "src"));
  } else if (strcasecmp(name, "iframe") == 0) {
    output("iframe", NULL, attval(attribs, "src"));
  } else if (strcasecmp(name, "head") == 0) {
    output("head", NULL, attval(attribs, "profile"));
  } else if (strcasecmp(name, "script") == 0) {
    output("script", NULL, attval(attribs, "src"));
  } else if (strcasecmp(name, "body") == 0) {
    output("body", NULL, attval(attribs, "background"));
  }

  /* Free memory */
  pairlist_delete(attribs);
  free(name);
}

/* handle_emptytag -- called after an empty tag is parsed */
void handle_emptytag(void *clientdata, string name, pairlist attribs)
{
  handle_starttag(clientdata, name, attribs);
}

/* handle_endtag -- called after an endtag is parsed (name may be "") */
void handle_endtag(void *clientdata, string name)
{
  free(name);
}

/* --------------------------------------------------------------------- */


/* usage -- print usage message and exit */
static void usage(string progname)
{
  fprintf(stderr,
	  "Version %s\nUsage: %s [-l] [-r] [-h] [-b base] [-t] [HTML-file]\n",
	  VERSION, progname);
  exit(1);
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
      case 'l': format = Long; break;		/* Long listing */
      case 'b': base = strdup(argv[++i]); break; /* Set base of URL */
      case 'r': relative = True; break;		/* Do not make URLs absolute */
      case 'h': format = HTML; break;		/* Output in HTML format */
      case 't': format = Tuple; break;		/* Output as 4-tuples */
      default: usage(argv[0]);
    }
  }
  if (i < argc && eq(argv[i], "--")) i++;

  if (i == argc) {
    yyin = stdin;
    self = "-";
  } else if (i == argc - 1) {
    if (!base) base = strdup(argv[i]);
    if (eq(argv[i], "-")) yyin = stdin; else yyin = fopenurl(argv[i], "r");
    self = argv[i];
  } else {
    usage(argv[0]);
  }

  if (yyin == NULL) {perror(argv[i]); exit(1);}

  if (yyparse() != 0) exit(3);

  if (base) free(base);

  return has_error ? 1 : 0;
}
