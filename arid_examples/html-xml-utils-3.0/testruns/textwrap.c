/*
 * Routines to wrap lines and indent them.
 *
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Bert Bos
 * Created 10 May 1998
 * $Id: textwrap.c,v 1.17 2004/04/26 12:34:24 bbos Exp $
 */
#include <config.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#if STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
#endif
#include "export.h"
#include "types.e"
#include "errexit.e"

/* To do: get rid of this arbitrary limit */
#define MAXLINE 32768
#define NBSP 128				/* Marks non-break-space */

static unsigned char buf[MAXLINE];
static int len = 0;				/* Length of buf */
static int linelen = 0;				/* Length of printed line */
static int level = 0;				/* Indentation level */
static int indent = 2;				/* # of spaces per indent */
static int maxlinelen = 72;			/* Desired line length */
static char prev = NBSP;			/* Previously added char */

/* set_indent -- set the amount of indent per level */
EXPORT void set_indent(int n) {indent = n;}

/* set_linelen -- set the maximum length of a line */
EXPORT void set_linelen(int n) {maxlinelen = n;}

/* flush -- print word in buf */
EXPORT void flush()
{
  int i, j;

  assert(len <= sizeof(buf));
  while (len != 0 && linelen + len >= maxlinelen) { /* Line needs break */
    /* Find last space before maxlinelen */
    for (i = maxlinelen - linelen - 1; i >= 0 && buf[i] != ' '; i--) ;
    /* If none, find first space after maxlinelen, or end of buffer */
    if (i < 0)
      for (i = linelen <= maxlinelen ? maxlinelen - linelen : 0;
	   i < len && buf[i] != ' '; i++) ;
    if (i == len) break;			/* No breakpoint */
    assert(i >= 0);				/* Found a breakpoint at i */
    assert(buf[i] == ' ');
    /* Print up to breakpoint (removing non-break-space markers) */
    for (j = 0; j < i; j++) putchar(buf[j] != NBSP ? buf[j] : ' ');
    putchar('\n');				/* Break line */
    linelen = 0;
    assert(level >= 0);
    assert(len >= 0);
    assert(i <= len);
    i++;					/* Skip the breakpoint */
#if 0
    while (i < len && buf[i] == ' ') i++;	/* Skip any subseq. spaces */
#endif
    memmove(buf + level * indent, buf + i, len - i);
    for (j = 0; j < level * indent; j++) buf[j] = NBSP; /* Indent */
    len += level * indent - i;
  }
  /* Print rest, if any (removing non-break-space markers) */
  /* First remove spaces at end of line */
  while (len > 0 && buf[len-1] == ' ') len--;
  for (j = 0; j < len; j++) putchar(buf[j] != NBSP ? buf[j] : ' ');
  linelen += len;
  len = 0;
}

/* outc -- add one character to output buffer */
EXPORT void outc(unsigned char c, Boolean preformatted)
{
  if (c == '\n' && !preformatted) c = ' ';	/* Newline is just a space */
  if (c == '\r' && !preformatted) c = ' ';	/* CR is just a space */
  if (c == '\t' && !preformatted) c = ' ';	/* Tab is just a space */
  if (c == '\f' && !preformatted) c = ' ';	/* Formfeed is just a space */
  if (c == ' ' && preformatted) c = NBSP;	/* Non-break-space marker */
  if (c == ' ' && prev == ' ') return;		/* Don't add another space */
  if (c == ' ' && linelen + len >= maxlinelen) flush();	/* Empty the buf */
  if (c == '\n' || c == '\r' || c == '\f') flush(); /* Empty the buf */
  if (c == ' ' && linelen + len == 0) return;	/* No ins at BOL */
  if (level * indent >= MAXLINE) errexit("Buffer overflow, sorry\n"); /* Hmm */
  if (linelen + len == 0) while (len < level * indent) buf[len++] = NBSP;
  if (c == ' ' && len && buf[len-1] == ' ') return; /* Skip multiple spaces */
  if (len >= MAXLINE) errexit("Buffer overflow, sorry\n"); /* Hmm... */
  buf[len++] = c;				/* Finally, insert c */
  prev = c;					/* Remember for next round */
}

/* out -- add text to current output line, print line if getting too long */
EXPORT void out(unsigned char *s, Boolean preformatted)
{
  if (s) for (; *s; s++) outc(*s, preformatted);
}

/* outn -- add n chars to current output, print line if getting too long */
EXPORT void outn(unsigned char *s, size_t n, Boolean preformatted)
{
  size_t i;
  for (i = 0; i < n; i++) outc(s[i], preformatted);
}

/* outln -- add string to output buffer, followed by '\n' */
EXPORT void outln(unsigned char *s, Boolean preformatted)
{
  out(s, preformatted);
  flush();
  assert(len == 0);
  putchar('\n');
  linelen = 0;
}

/* outbreak -- conditional new line; make sure next text starts on new line */
EXPORT void outbreak()
{
  flush();
  assert(len == 0);
  if (linelen != 0) {
    putchar('\n');
    linelen = 0;
  }
}

/* inc_indent -- increase indentation level by 1 */
EXPORT void inc_indent(void)
{
  flush();
  level++;
}

/* decc_indent -- decrease indentation level by 1 */
EXPORT void dec_indent(void)
{
  flush();
  level--;
}
