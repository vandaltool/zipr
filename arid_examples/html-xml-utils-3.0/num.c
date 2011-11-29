/*
 * Number headers. Counters are inserted at the start
 * of H1 to H6. CLASS="no-num" suppresses numbering for
 * that heading.
 *
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Bert Bos
 * Created Sep 1997
 * $Id: num.c,v 1.16 2000/08/07 12:28:44 bbos Exp $
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

#define SECNO "secno"				/* class attribute */
#define NO_NUM "no-num"				/* class-attribute */

static int h[] = {-1, 0, 0, 0, 0, 0, 0};	/* Counters for each level */
static int low = 1;				/* First counter to use */
static int high = 6;				/* Last counter to use */
static unsigned char *format[7] = {			/* Format for each counter */
  NULL, "%d.", "%d.%d.", "%d.%d.%d.", "%d.%d.%d.%d.",
  "%d.%d.%d.%d.%d.", "%d.%d.%d.%d.%d.%d."};
static int skipping = 0;			/* >0 to suppress output */


/* romannumeral -- generate roman numeral for 1 <= n <= 4000 */
static unsigned char* romannumeral(int n)
{
  static unsigned char buf[30];
  int len = 0;

  while (n >= 1000) {buf[len++] = 'M'; n -= 1000;}
  if (n >= 500) {buf[len++] = 'D'; n -= 500;}
  while (n >= 100) {buf[len++] = 'C'; n -= 100;}
  if (n >= 50) {buf[len++] = 'L'; n -= 50;}
  while (n >= 10) {buf[len++] = 'X'; n -= 10;}
  if (n >= 9) {buf[len++] = 'I'; buf[len++] = 'X'; n -= 9;}
  if (n >= 5) {buf[len++] = 'V'; n -= 5;}
  if (n >= 4) {buf[len++] = 'I'; buf[len++] = 'V'; n -= 4;}
  while (n >= 1) {buf[len++] = 'I'; n -= 1;}
  buf[len] = '\0';
  return buf;
}

/* down -- convert a string to lowercase, return pointer to start of string */
static unsigned char* down(unsigned char *t)
{
  unsigned char *s;
  for (s = t; *s; s++) *s = tolower(*s);
  return t;
}

/* --------------- implements interface api.h -------------------------- */

/* handle_error -- called when a parse error occurred */
void handle_error(void *clientdata, const string s, int lineno)
{
  fprintf(stderr, "%d: %s\n", lineno, s);
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
void handle_comment(void *clientdata, unsigned char *commenttext)
{
  printf("<!--%s-->", commenttext);
}

/* handle_text -- called after a text chunk is parsed */
void handle_text(void *clientdata, unsigned char *text)
{
  if (skipping == 0) fputs(text, stdout);
}

/* handle_decl -- called after a declaration is parsed */
void handle_decl(void *clientdata, unsigned char *gi,
		 unsigned char *fpi, unsigned char *url)
{
  if (skipping == 0) {
    printf("<!DOCTYPE %s ", gi);
    if (fpi) printf("PUBLIC \"%s\"\n", fpi); else printf("SYSTEM");
    if (url) printf(" \"%s\"", url);
    printf(">");
  }
}

/* handle_proc_instr -- called after a PI is parsed */
void handle_pi(void *clientdata, unsigned char *pi_text)
{
  if (skipping == 0) printf("<?%s>", pi_text);
}

/* contains -- check if string contains a certain word, return pointer */
static unsigned char* contains(unsigned char *s, unsigned char *word)
{
  unsigned char c, *t;
  t = strstr(s, word);
  if (t == NULL)
    return NULL;				/* Not found */
  if (t != s && !isspace(*(t - 1)))
    return NULL;				/* Not beginning of word */
  if ((c = *(t + strlen(word))) != '\0' && !isspace(c))
    return NULL;				/* Not end of word */
  return t;
}

/* Check for class=secno in list of attributes */
static Boolean class_contains_secno(pairlist attribs)
{
  pairlist p;

  for (p = attribs; p != NULL; p = p->next) {
    if (strcasecmp(p->name, "class") == 0 && contains(p->value, SECNO))
      return True;
  }
  return False;
}

/* Check for class=no-num in list of attributes */
static Boolean class_contains_no_num(pairlist attribs)
{
  pairlist p;

  for (p = attribs; p != NULL; p = p->next) {
    if (strcasecmp(p->name, "class") == 0 && contains(p->value, NO_NUM))
      return True;
  }
  return False;
}

/* handle_starttag -- called after a start tag is parsed */
void handle_starttag(void *clientdata, unsigned char *name, pairlist attribs)
{
  pairlist p;
  int lev, i;
  unsigned char *s;

  /* Skip everything inside <span class=secno> */
  if (skipping > 0) {
    skipping++;
    return;
  }

  /* Check for old counters, skip them */
  if (strcasecmp(name, "span") == 0 && class_contains_secno(attribs)) {
    skipping = 1;
    return;
  }

  /* Print tag and attributes */
  printf("<%s", name);
  for (p = attribs; p != NULL; p = p->next) {
    printf(" %s", p->name);
    if (p->value != NULL) printf("=\"%s\"", p->value);
  }
  printf(">");

  /* If header, insert counters */
  if (eq("h1", name) || eq("H1", name)) lev = 1;
  else if (eq("h2", name) || eq("H2", name)) lev = 2;
  else if (eq("h3", name) || eq("H3", name)) lev = 3;
  else if (eq("h4", name) || eq("H4", name)) lev = 4;
  else if (eq("h5", name) || eq("H5", name)) lev = 5;
  else if (eq("h6", name) || eq("H6", name)) lev = 6;
  else lev = 0;

  /* Don't number headers with class "no-num" */
  if (lev > 0 && class_contains_no_num(attribs)) lev = 0;

  if (low <= lev && lev <= high) {
    h[lev]++;
    for (i = lev + 1; i <= high; i++) h[i] = 0;
    printf("<span class=\"%s\">", SECNO);
    for (i = low, s = format[lev]; *s; s++) {
      if (*s == '%') {
	s++;
	switch (*s) {
	  case 'n': i++; break;			/* No number */
	  case 'd': printf("%d", h[i++]); break; /* Decimal */
	  case 'a': printf("%c", 'a' + (h[i++] - 1)); break; /* Lowercase */
	  case 'A': printf("%c", 'A' + (h[i++] - 1)); break; /* Uppercase */
	  case 'i': printf("%s", down(romannumeral(h[i++]))); break;
	  case 'I': printf("%s", romannumeral(h[i++])); break; /* Roman */
	  default: putchar(*s);			/* Escaped char */
	}
      } else {
	putchar(*s);
      }
    }
    printf(" </span>");
  }
}

/* handle_emptytag -- called after an empty tag is parsed */
void handle_emptytag(void *clientdata, unsigned char *name, pairlist attribs)
{
  pairlist p;

  if (skipping == 0) {
    printf("<%s", name);
    for (p = attribs; p != NULL; p = p->next) {
      printf(" %s", p->name);
      if (p->value != NULL) printf("=\"%s\"", p->value);
    }
    printf(" />");
  }
}

/* handle_endtag -- called after an endtag is parsed (name may be "") */
void handle_endtag(void *clientdata, unsigned char *name)
{
  if (skipping == 0) printf("</%s>", name);
  else skipping--;
}

/* --------------------------------------------------------------------- */

/* usage -- print usage message and exit */
static void usage(unsigned char *prog)
{
  fprintf(stderr, "Version %s\n\
Usage: %s [-l low] [-h high] [-1 format] [-2 format] [-3 format]\n\
  [-4 format] [-5 format] [-6 format] [html-file]\n", VERSION, prog);
  exit(2);
}

/* help -- print help */
static void help(void)
{
  printf("Version %s\n", VERSION);
  printf("Options:\n");
  printf("  -l low     lowest header level to number (1-6) [default 1]\n");
  printf("  -h high    highest header level to number (1-6) [default 6]\n");
  printf("  -n start   number of first heading [default: 1]\n");
  printf("  -1 format  format for level 1 [default \"%%d.\"]\n");
  printf("  -2 format  format for level 2 [default \"%%d.%%d.\"]\n");
  printf("  -3 format  format for level 3 [default \"%%d.%%d.%%d.\"]\n");
  printf("  -4 format  format for level 4 [default \"%%d.%%d.%%d.%%d.\"]\n");
  printf("  -5 format  format for level 5 [default \"%%d.%%d.%%d.%%d.%%d.\"]\n");
  printf("  -6 format  format for level 6 [default \"%%d.%%d.%%d.%%d.%%d.%%d.\"]\n");
  printf("  -?         this help\n");
  printf("The format strings may contain:\n");
  printf("  %%d  replaced by decimal number\n");
  printf("  %%a  replaced by letter a, b, c,..., z\n");
  printf("  %%A  replaced by letter A, B, C,..., Z\n");
  printf("  %%i  replaced by lowercase roman numeral i, ii, iii,...\n");
  printf("  %%I  replaced by roman numeral I, II, III,...\n");
  printf("  %%n  replaced by nothing, but skips a level\n");
  printf("  %%%%  replaced by a %%\n");
  printf("The first %% in the format is replaced by the counter for level\n");
  printf("low, the second by the counter for low+1, etc.\n");
  exit(0);
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

  /* First find -l and -h */
  for (i = 1; i < argc && argv[i][0] == '-' && !eq(argv[i], "--"); i++) {
    switch (argv[i][1]) {
      case 'l': low = atoi(argv[++i]); break;
      case 'h': high = atoi(argv[++i]); break;
      default: ;
    }
  }
  /* If -l and/or -h have been set, the default formats are different */
  if (low != 1 || high != 6) {
    for (i = high; i >= low; i--) format[i] = format[i-low+1];
    for (i = high + 1; i <= 6; i++) format[i] = "";
  }
  /* Then treat other options */
  for (i = 1; i < argc && argv[i][0] == '-' && !eq(argv[i], "--"); i++) {
    switch (argv[i][1]) {
      case 'l': i++; break;			/* Already handled */
      case 'h': i++; break;			/* Already handled */
      case 'n': h[low] = atoi(argv[++i]) - 1; break;
      case '1': format[1] = argv[++i]; break;
      case '2': format[2] = argv[++i]; break;
      case '3': format[3] = argv[++i]; break;
      case '4': format[4] = argv[++i]; break;
      case '5': format[5] = argv[++i]; break;
      case '6': format[6] = argv[++i]; break;
      case '?': help(); break;
      default: usage(argv[0]);
    }
  }
  if (i < argc && eq(argv[i], "--")) i++;

  if (i == argc) yyin = stdin;
  else if (i == argc - 1 && eq(argv[i], "-")) yyin = stdin;
  else if (i == argc - 1) yyin = fopenurl(argv[i], "r");
  else usage(argv[0]);

  if (yyin == NULL) {perror(argv[i]); exit(1);}

  if (yyparse() != 0) {
    exit(3);
  }
  return 0;
}
