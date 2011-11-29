/*
 * unpipe - takes output of pipe and convert to HTML/XML form
 *
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 23 May 1999
 * Version: $Id: unpipe.c,v 1.9 2003/01/21 19:26:03 bbos Exp $
 */
#include <config.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <assert.h>
#ifdef HAVE_STRING_H
#  include <string.h>
#elif HAVE_STRINGS_H
#  include <strings.h>
#endif
#include "export.h"
#include "types.e"
#include "heap.e"
#include "errexit.e"
#include "openurl.e"

#define MAXSTRING 4096

static Boolean do_xml = False;
static int nrattrs = 0;
static char **attrs = NULL;

/* put_text -- replace newlines and print text */
static void put_text(char *buf)
{
  int i;

  for (i = 0; buf[i]; i++) {
    if (buf[i] != '\\') {
      putchar(buf[i]);
    } else {
      i++;
      switch (buf[i]) {
	case 'n': putchar('\n'); break;
	case 'r': putchar('\r'); break;
	case 't': putchar('\t'); break;
	default: putchar(buf[i]);
      }
    }
  }
}

/* store_attr -- store attributes temporarily */
static void store_attr(const char *buf)
{
  assert(buf[0] == 'A');
  renewarray(attrs, ++nrattrs);
  attrs[nrattrs-1] = newstring(buf);
}

/* put_attr -- write out attributes */
static void put_attr(void)
{
  int i, j;

  for (j = 0; j < nrattrs; j++) {
    putchar(' ');
    for (i = 1; attrs[j][i] != ' '; i++) putchar(attrs[j][i]);
    putchar('=');
    for (i++; attrs[j][i] != ' '; i++) ;	/* skip type ("CDATA") */
    putchar('"');
    put_text(attrs[j] + i + 1);
    putchar('"');
    dispose(attrs[j]);
  }
  nrattrs = 0;
}

/* put_decl -- write a DOCTYPE declaration */
static void put_decl(const char *buf)
{
  int i;

  assert(buf[0] == '!');
  printf("<!DOCTYPE ");
  /* write name of root element */
  for (i = 1; buf[i] && buf[i] != ' '; i++) putchar(buf[i]);
  if (! buf[i]) errexit("Incorrect DOCTYPE declaration: %s\n", buf);
  if (buf[++i] != '"') errexit("Incorrect DOCTYPE declaration: %s\n", buf);
  /* write FPI if present */
  if (buf[++i] != '"') {			/* FPI */
    printf(" PUBLIC \"");
    for (; buf[i] && buf[i] != '"'; i++) putchar(buf[i]);
    putchar('"');
  } else {					/* No FPI */
    printf(" SYSTEM");
  }
  if (! buf[i]) errexit("Incorrect DOCTYPE declaration: %s\n", buf);
  i++;
  if (buf[++i]) printf(" \"%s\"", buf + i);	/* URL */
  printf(">");
}

/* usage -- print usage message and exit */
static void usage(unsigned char *prog)
{
  fprintf(stderr, "Version %s\nUsage: %s [-x] [file_or_url]\n", VERSION, prog);
  exit(1);
}

int main(int argc, char *argv[])
{
  int c;
  char buf[MAXSTRING];
  FILE *in = NULL;

  while ((c = getopt(argc, argv, "x")) != -1)
    switch (c) {
      case 'x': do_xml = True; break;
      default: usage(argv[0]);
    }
  if (optind == argc) in = stdin;
  else if (optind == argc - 1) in = fopenurl(argv[optind], "r");
  else usage(argv[0]);
  if (in == NULL) { perror(argv[optind]); exit(2); }

  /* ToDo: recognize empty elements */

  while (fgets(buf, sizeof(buf), in)) {
    buf[strlen(buf)-1] = '\0';			/* Remove newline */
    switch (buf[0]) {
      case '-': put_text(buf + 1); break;
      case '?': printf("<?"); put_text(buf + 1); printf(">"); break;
      case '*': printf("<!--"); put_text(buf + 1); printf("-->"); break;
      case 'L': break;
      case 'A': store_attr(buf); break;
      case '(': printf("<%s", buf + 1); put_attr(); putchar('>'); break;
      case ')': printf("</%s>", buf + 1); break;
      case '|': printf("<%s", buf + 1); put_attr(); printf(" />"); break;
      case '!': put_decl(buf); break;
      default: errexit("Unknown code at start of line: %c\n", buf[0]);
    }
  }
  if (! feof(in)) { perror(argv[0]); exit(1); }
  fclose(in);
  return 0;
}
