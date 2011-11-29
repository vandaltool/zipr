%option 8bit caseless nodefault noyywrap
/* %option yylineno */

%{
/*
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 1997
 * Version: $Id: scan.l,v 1.27 2004/04/26 12:34:10 bbos Exp $
 **/
#include <config.h>
#include <assert.h>
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
#include <stdlib.h>
#include <ctype.h>
#include "export.h"
#include "types.e"
#include "heap.e"
#include "html.h"
#include "errexit.e"


EXPORT extern FILE *yyin;
EXPORT int yylineno = 1; 

typedef struct _Stack {
  YY_BUFFER_STATE buf;
  FILE *f;
  struct _Stack *next;
} *Stack;

static Stack stack = NULL;


/* include_file -- stack current file and switch to another one */
EXPORT void include_file(FILE *f)
{
  Stack h;

  new(h);
  h->buf = YY_CURRENT_BUFFER;
  h->f = f;
  h->next = stack;
  stack = h;
  yy_switch_to_buffer(yy_create_buffer(f, YY_BUF_SIZE));
}

/* pop_file -- back to previous input file */
static Boolean pop_file(void)
{
  Stack h;

  if (!stack) {
    return False;
  } else {
    h = stack;
    yy_delete_buffer(YY_CURRENT_BUFFER);
    fclose(h->f);
    yy_switch_to_buffer(h->buf);
    stack = h->next;
    dispose(h);
    return True;
  }
}

/* esc -- remove outer quotes, escape ", remove \n, return malloc'ed string */
static unsigned char* esc(unsigned char* s)
{
  int i, j;
  unsigned char *u;

  /* Find new length */
  for (i = 0, j = 1; s[j] != s[0]; i++, j++) {
    if (s[j] == '"') i+= 4;
  }
  /* Copy and expand */
  u = malloc(i + 1);
  if (!u) errexit("Out of memory\n");
  for (i = 0, j = 1; s[j] != s[0]; i++, j++) {
    if (s[j] == '"')  {strcpy(u + i, "&#34;"); i += 4;}
    else if (s[j] == '\n') u[i] = ' ';		/* \n */
    else if (s[j] == '\r' && s[j+1] == '\n') {u[i] = ' '; j++;}	/* \r\n */
    else if (s[j] == '\r') {u[i] = ' ';}	/* \r */
    else u[i] = s[j];
  }
  u[i] = '\0';
  return u;
}
 
/* u_strndup -- allocate a string, copy n characters into it and add \0 */
static unsigned char* u_strndup(const unsigned char *s, size_t n)
{
  unsigned char *t = malloc(n + 1);
  if (!t) errexit("Out of memory\n");
  strncpy(t, s, n);
  t[n] = '\0';
  return t;
}

/* lns -- count newlines */
static void lns(const unsigned char *s)
{
  while (*s) {
    if (*s == '\n') yylineno++;
    else if (*s != '\r') ;
    else if (*(s+1) == '\n') {yylineno++; s++;}
    else yylineno++;
    s++;
  }
}

%}

/* thing is rather too permissive, but it will accept <img src=/path>... */

delim		[ \t\r\n\f="'<>]
nondelim	[^ \t\r\n\f="'<>]
name		[a-z0-9:._-]+|"{"[^} \t\r\n\f]*"}"[a-z0-9:._-]+
thing		{nondelim}+
comment		"<!--"([^-]|-[^-]|--[^>])*"-->"
data		([^<\r\n]|<[^a-z/?!{])+
doctype		"<!doctype"[ \t\r\n\f]
nl		\n|\r\n|\r

%s MARKUP VALUE DECL INIT

%%


<INITIAL>\357\273\277		{BEGIN(INIT); /* Byte Order Mark is ignored */}

<INITIAL,INIT>"<"{name}		{BEGIN(MARKUP); yylval.s=strdup(yytext+1); return START;}
<INITIAL,INIT>"</"({name})?	{BEGIN(MARKUP); yylval.s=strdup(yytext+2); return END;}
<INITIAL,INIT>{data}		{yylval.s=strdup(yytext); return TEXT;}
<INITIAL,INIT>{nl}		{yylval.s=strdup(yytext); yylineno++; return TEXT;}
<INITIAL,INIT>{comment}	{yylval.s=u_strndup(yytext+4,yyleng-7); lns(yytext); return COMMENT;}
<INITIAL,INIT>{doctype}	{BEGIN(DECL); lns(yytext+9); return DOCTYPE;}
<INITIAL,INIT>"<?"[^>]*">"	{yylval.s=u_strndup(yytext+2,yyleng-3); lns(yytext); return PROCINS;}

<MARKUP>{name}		{yylval.s = strdup(yytext); return NAME;}
<MARKUP>"="		{BEGIN(VALUE); return '=';}
<MARKUP>[ \t\f]+	{; /* skip */}
<MARKUP>{nl}		{yylineno++; /* skip */}
<MARKUP>">"		{BEGIN(INIT); return '>';}
<MARKUP>"/>"		{BEGIN(INIT); return EMPTYEND;}
<MARKUP>"<"		{BEGIN(INIT); yyless(0); return '>'; /* Implicit ">" */} 

<VALUE>[ \t\f]+		{; /* skip */}
<VALUE>{nl}		{yylineno++; /* skip */}
<VALUE>{thing}		{BEGIN(MARKUP); yylval.s=strdup(yytext); return NAME;}
<VALUE>\"[^"]*\"	|
<VALUE>\'[^']*\'	{BEGIN(MARKUP); yylval.s=esc(yytext); lns(yytext); return STRING;}

<DECL>{name}		{yylval.s = strdup(yytext); return NAME;}
<DECL>[ \t\f]+		{; /* skip */}
<DECL>{nl}		{yylineno++; /* skip */}
<DECL>\"[^"]*\"		|
<DECL>\'[^']*\'		{lns(yytext); yylval.s = esc(yytext); return STRING;}
<DECL>">"		{BEGIN(INIT); return '>';}

.			{return *yytext; /* illegal char, in fact */}

<<EOF>>			{if (pop_file()) return ENDINCL; else yyterminate();}

%%
/*
 * Local variables:
 * mode: indented-text
 * End:
 */
