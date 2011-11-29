/*
 *
 * Program to convert files from ASCII or ISO-8859-1 to UTF8.
 *
 * Usage:
 *
 *   asc2xml
 *
 * Reads from stdin and write to stdout. Converts from ASCII (in fact:
 * Latin-1) (with or without &#-escapes) to UTF8, removing all
 * &#-escapes, except those representing ASCII characters.
 *
 * Version: $Revision: 1.2 $ ($Date: 2003/01/21 19:19:31 $)
 * Author: Bert Bos <bert@w3.org>
 *
 * Copyright © 1994-2002 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 **/
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#if STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
#endif
#include <ctype.h>

/* putUTF8 -- write a character to stdout in UTF8 encoding */
static void putUTF8(long c)
{
  if (c <= 0x7F) {				/* Leave ASCII encoded */
    printf("&#%ld;", c);
  } else if (c <= 0x07FF) {			/* 110xxxxx 10xxxxxx */
    putchar(0xC0 | (c >> 6));
    putchar(0x80 | (c & 0x3F));
  } else if (c <= 0xFFFF) {			/* 1110xxxx + 2 */
    putchar(0xE0 | (c >> 12));
    putchar(0x80 | ((c >> 6) & 0x3F));
    putchar(0x80 | (c & 0x3F));
  } else if (c <= 0x1FFFFF) {			/* 11110xxx + 3 */
    putchar(0xF0 | (c >> 18));
    putchar(0x80 | ((c >> 12) & 0x3F));
    putchar(0x80 | ((c >> 6) & 0x3F));
    putchar(0x80 | (c & 0x3F));
  } else if (c <= 0x3FFFFFF) {			/* 111110xx + 4 */
    putchar(0xF8 | (c >> 24));
    putchar(0x80 | ((c >> 18) & 0x3F));
    putchar(0x80 | ((c >> 12) & 0x3F));
    putchar(0x80 | ((c >> 6) & 0x3F));
    putchar(0x80 | (c & 0x3F));
  } else if (c <= 0x7FFFFFFF) {			/* 1111110x + 5 */
    putchar(0xFC | (c >> 30));
    putchar(0x80 | ((c >> 24) & 0x3F));
    putchar(0x80 | ((c >> 18) & 0x3F));
    putchar(0x80 | ((c >> 12) & 0x3F));
    putchar(0x80 | ((c >> 6) & 0x3F));
    putchar(0x80 | (c & 0x3F));
  } else {					/* Not a valid character... */
    printf("&#%ld;", c);
  } 
}

/* asc2xml -- copy stdin to stdout, converting ASCII XML to UTF8 XML */
static void asc2xml(void)
{
  long n;
  int c;
  while ((c = getchar()) != EOF) {
    if (c > 0x7F) {				/* Latin-1, non-ASCII */
      putUTF8(c);
    } else if (c != '&') {			/* Normal ASCII char */
      putchar(c);
    } else if ((c = getchar()) == EOF) {	/* '&' before EOF */
      putchar('&');
    } else if (c != '#') {			/* '&' not followed by '#' */
      putchar('&');
      putchar(c);
    } else if ((c = getchar()) == 'x') {	/* '&#x' + hexadecimal */
      n = 0;
      while (isxdigit((c = getchar()))) {
	if (c <= '9') n = 16 * n + c - '0';
	else if (c <= 'F') n = 16 * n + c - 'A' + 10;
	else n = 16 * n + c - 'a' + 10;
      }
      /* Don't check for overflow, don't check if c == ';' */
      putUTF8(n);
    } else {					/* '&#' + decimal */
      n = c - '0';
      while (isdigit((c = getchar()))) {
	n = 10 * n + c - '0';
      }
      /* Don't check for overflow, don't check if c == ';' */
      putUTF8(n);
    }
  }
}

/* Print usage message, then exit */
static void usage(char *progname)
{
  fprintf(stderr, "Version %s\nUsage: %s <infile >outfile\n", VERSION, progname);
  exit(1);
}

/* main -- main body */
int main(int argc, char *argv[])
{
  if (argc != 1) usage(argv[0]);
  asc2xml();
  return 0;
}
