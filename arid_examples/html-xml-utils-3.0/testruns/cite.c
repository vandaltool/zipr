/*
 * cite - adds hyperlinks to bibliographic references in HTML
 *
 * The programs looks for strings of the form [[name]] (i.e., a
 * bibliographic label inside a double pair of square brackets), e.g.,
 * [[Knuth84]] or [[LieBos97]]. The label will be looked up in a
 * bibliography database and if it is found, the string will be
 * replaced by a pattern which is typically of the form <a
 * href="...">[name]</a>, but the pattern can be changed
 * with a command line option.
 *
 * If the label is not found, a warning is printed and the string is
 * left unchanged.
 *
 * All labels that are found are also stored, one label per line, in a
 * separate file with extension .aux. This file can be used by mkbib
 * to create the bibliography by extracting the corresponding
 * bibliographic entries from the database.
 *
 * The bibliography database must be a refer-style database. Though
 * for the purposes of this program all lines that don't start with
 * "%L" are ignored. Lines with "%L" are assumed to contain a label.
 *
 * Options:
 *
 * -b base
 *     Give the value for %b in the pattern.
 *
 * -p pattern
 *     The replacement for the string [[label]]. The default is
 *
 *     <a href=\"%b#%L\" rel=\"biblioentry\">[%L]</a>
 *
 *     %L will be replaced by the label, %b by the value of the -b
 *     option.
 *
 * -a auxfile
 *     The name of the file in which the list of labels will be stored.
 *     Default is the name of the file given as argument, minus its
 *     extension, plus ".aux". If no file is give (input comes from
 *     stdin), the default name is "aux.aux".
 *
 * -m marker
 *     By default, the program looks for "[[name]]", but it can be
 *     made to look for "[[Xname]]" where X is some string, usually a
 *     symbol such as '!' or ='. This allows references to be
 *     classified, e.g., "[[!name]]" for normative references and
 *     "[[name]]" for non-normative references.
 *
 * Copyright © 1994-2002 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 18 March 2000
 * Version: $Id: cite.c,v 1.17 2004/04/29 15:54:24 bbos Exp $
 **/

#include <config.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
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
#  include "hash.e"
#endif

#include <ctype.h>
#include "export.h"
#include "heap.e"
#include "types.e"
#include "errexit.e"


/* Warning: arbitrary limits! */
#define LINESIZE 32768
#define HASHSIZE 4096				/* Size of hash table */

static string base = "";			/* URL of bibilography */
static string mark = "";			/* Flag after "'[[" */
static size_t marklen = 0;			/* Length of mark */
static string prog;				/* = argv[0] */
static string pattern = "<a href=\"%b#%L\" rel=\"biblioentry\">[%L]</a>";
static FILE *aux;


/* label_exists -- check if the label exists in the bibliographic database */
static Boolean label_exists(const string label)
{
  ENTRY e = {label, NULL};

  return hsearch(e, FIND) != NULL;
}


/* valid_label -- check if the label is well-formed */
static Boolean valid_label(const string label)
{
  int i;

  for (i = 0; label[i]; i++)
    if (! isalnum(label[i])
	&& label[i] != '-'
	&& label[i] != '_'
	&& label[i] != '.') return False;
  return True;
}


/* expand_ref -- print the reformatted reference */
static void expand_ref(const string label)
{
  int i;

  /* ToDo: somehow allow sequence numbers for references [1], [2], etc. */
  for (i = 0; pattern[i]; i++) {
    if (pattern[i] != '%') {
      putchar(pattern[i]);
    } else {
      switch (pattern[++i]) {
	case '%': putchar('%'); break;		/* Literal '%' */
	case 'b': printf("%s", base); break;	/* Base URL */
	case 'L': printf("%s", label); break;	/* Label */
	default: break;				/* Error in pattern */
      }
    }
  }
}


/* process_line -- look for citations in a line */
EXPORT void process_line(const string text, const string fname, int lineno)
{
  string h = text, p, q, label;

  /* Loop over occurrences of "[[" + mark + label + "]]" */
  while ((p = strstr(h, "[[")) && (q = strstr(p, "]]"))) {

    while (h != p) putchar(*(h++));		/* Print text up to "[[" */

    if (marklen == 0 || strncmp(p + 2, mark, marklen) == 0) {

      p += 2 + marklen;				/* Skip "[[" + mark */
      label = newnstring(p, q - p);		/* Extract the label */

      if (! valid_label(label)) {		/* Cannot be a label */
	while (h != q) putchar(*(h++));		/* Copy unchanged */
	printf("]]");
      } else if (label_exists(label)) {		/* Citation found */
	expand_ref(label);			/* Insert full reference */
	fprintf(aux, "%s\n", label);		/* Store label */
      } else {					/* Label not found: warn */
	while (h != q) putchar(*(h++));		/* Copy unchanged */
	printf("]]");
	fprintf(stderr, "%s:%d: warning: no bib entry found for %s\n",
		fname ? fname : (string)"<stdin>", lineno, label);
      }
      dispose(label);

    } else {					/* No valid mark */

      while (h != q) putchar(*(h++));		/* Copy unchanged */
      printf("]]");
    }
    h = q + 2;
  }

  printf("%s", h);				/* Print rest of text */
}


/* parse_db -- extract all labels from the refer-style database */
static void parse_db(const string db)
{
  unsigned char line[LINESIZE];
  FILE *f;
  int e, i;
  ENTRY entry;

  if (!(f = fopen(db,"r"))) errexit("%s: %s: %s\n", prog, db, strerror(errno));

  /* Initialize the hash table */
  if (! hcreate(HASHSIZE)) errexit("%s: %s\n", prog, strerror(errno));

  /* Search for %L lines */
  clearerr(f);
  while (fgets(line, sizeof(line), f)) {
    if (strncmp(line, "%L ", 3) == 0) {
      for (i = strlen(line); i > 0 && isspace(line[i-1]); i--) ;
      if (i > 3) {				/* Ignore empty field */
	line[i] = '\0';
	entry.key = newstring(line + 3);
	if (!hsearch(entry, ENTER)) errexit("%s: %s\n", prog, strerror(errno));
      }
    }
  }
  if ((e = ferror(f))) errexit("%s: %s: %s\n", prog, db, strerror(e));

  if (fclose(f) != 0) errexit("%s: %s: %s\n", prog, db, strerror(errno));
}


/* usage -- print usage message and exit */
static void usage(void)
{
  errexit("Version %s\n\
Usage: %s [-b base] [-p pattern] [-a auxfile] bib-file [HTML-file]\n",
	  VERSION, prog);
}


int main(int argc, char *argv[])
{
  unsigned char line[LINESIZE];
  string h, auxfile = NULL, dbfile = NULL, infile = NULL;
  int i, e, lineno;
  FILE *f;

  /* Parse command line arguments */
  prog = argv[0];
  for (i = 1; i < argc && argv[i][0] == '-' && !eq(argv[i], "--"); i++) {
    switch (argv[i][1]) {
    case 'b': base = argv[++i]; break;	/* Set base of URL */
    case 'p': pattern = argv[++i]; break;	/* Form of expanded ref */
    case 'a': auxfile = argv[++i]; break;	/* Name of auxfile */
    case 'm': mark = argv[++i]; marklen = strlen(mark); break; /* After "[[" */
    default: usage();
    }
  }
  if (i < argc && eq(argv[i], "--")) i++;

  if (i == argc || argc > i + 2) usage();

  dbfile = argv[i++];
  if (i != argc) infile = argv[i++];

  /* Read the labels from the bibliography database */
  parse_db(dbfile);

  /* Construct auxfile */
  if (! auxfile) {
    if (infile) {
      newarray(auxfile, strlen(infile) + 5);
      strcpy(auxfile, infile);
      if ((h = strrchr(auxfile, '.'))) *h = '\0';
      strcat(auxfile, ".aux");
    } else {
      auxfile = "aux.aux";
    }
  }
  if (! (aux = fopen(auxfile, "w")))
    errexit("%s: %s: %s\n", prog, auxfile, strerror(errno));

  /* Open input file or use stdin */
  f = infile ? fopen(infile, "r") : stdin;
  if (!f) errexit("%s: %s: %s\n", prog, infile, strerror(errno));

  /* Read input line by line */
  clearerr(f);
  lineno = 1;
  while (fgets(line, sizeof(line), f)) process_line(line, infile, lineno++);
  if ((e = ferror(f))) errexit("%s: %s: %s\n", prog, argv[i], strerror(e));

  fclose(f);
  fclose(aux);
  return 0;
}
