/*
 *
 * Program to convert files from UTF-8 to ASCII and UTF8, using the
 * &#-escapes from XML to escape non-ASCII characters.
 *
 * Usage:
 *
 *   xml2asc
 *
 * Reads from stdin and write to stdout. Converts from UTF8 (with or
 * without &#-escapes) to ASCII, inserting &#-escapes for all
 * non-ASCII characters.
 *
 * Version: $Revision: 1.2 $ ($Date: 2003/01/21 19:26:03 $)
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

/* getUTF8 -- read a UTF8 encoded character from stdin */
static long getUTF8()
{
  long c;
  int b;
  if ((b = getchar()) == EOF) {			/* EOF */
    c = EOF;
  } else if (b <= 0x7F) {			/* ASCII */
    c = b;
  } else if ((b & 0xE0) == 0xC0) {		/* 110xxxxx 10xxxxxx */
    c = (b & 0x1F) << 6;
    b = getchar();				/* Don't check for 10xxxxxx */
    c |= b & 0x3F;
  } else if ((b & 0xF0) == 0xE0) {		/* 1110xxxx + 2 */
    c = (b & 0x0F) << 12;
    b = getchar();
    c |= (b & 0x3F) << 6;
    b = getchar();
    c |= b & 0x3F;
  } else if ((b & 0xF1) == 0xF0) {		/* 11110xxx + 3 */
    c = (b & 0x0F) << 18;
    b = getchar();
    c |= (b & 0x3F) << 12;
    b = getchar();
    c |= (b & 0x3F) << 6;
    b = getchar();
    c |= b & 0x3F;
  } else if ((b & 0xFD) == 0xF8) {		/* 111110xx + 4 */
    c = (b & 0x0F) << 24;
    b = getchar();
    c |= (b & 0x0F) << 18;
    b = getchar();
    c |= (b & 0x3F) << 12;
    b = getchar();
    c |= (b & 0x3F) << 6;
    b = getchar();
    c |= b & 0x3F;
  } else if ((b & 0xFE) == 0xFC) {		/* 1111110x + 5 */
    c = (b & 0x0F) << 30;
    b = getchar();
    c |= (b & 0x0F) << 24;
    b = getchar();
    c |= (b & 0x0F) << 18;
    b = getchar();
    c |= (b & 0x3F) << 12;
    b = getchar();
    c |= (b & 0x3F) << 6;
    b = getchar();
    c |= b & 0x3F;
  } else {
    c = 0;
    /* Error */
  }
  return c;
}

/* xml2asc -- copy stdin to stdout, converting UTF8 XML to ASCII XML */
static void xml2asc(void)
{
  long c;
  while ((c = getUTF8()) != EOF) {
    if (c <= 127)
      putchar(c);
    else
      printf("&#%ld;", c);
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
  xml2asc();
  return 0;
}
