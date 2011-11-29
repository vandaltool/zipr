%{
/*
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 1997
 * Version: $Id: html.y,v 1.17 2000/08/20 16:35:27 bbos Exp $
 **/
#include <config.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "export.h"
#include "types.e"
#include "tree.e"

/* The types of the various callback routines */

EXPORT typedef void (*html_handle_error_fn)
  (void *clientdata, const string s, int lineno);
EXPORT typedef void* (*html_handle_start_fn)
  (void);
EXPORT typedef void (*html_handle_end_fn)
  (void *clientdata);
EXPORT typedef void (*html_handle_comment_fn)
  (void *clientdata, const string commenttext);
EXPORT typedef void (*html_handle_text_fn)
  (void *clientdata, const string text);
EXPORT typedef void (*html_handle_decl_fn)
  (void *clientdata, const string gi, const string fpi, const string url);
EXPORT typedef void (*html_handle_pi_fn)
  (void *clientdata, const string pi_text);
EXPORT typedef void (*html_handle_starttag_fn)
  (void *clientdata, const string name, pairlist attribs);
EXPORT typedef void (*html_handle_emptytag_fn)
  (void *clientdata, const string name, pairlist attribs);
EXPORT typedef void (*html_handle_endtag_fn)
  (void *clientdata, const string name);
EXPORT typedef void (*html_handle_endincl_fn)
  (void *clientdata);

/* yyparse -- entry point for the parser */
EXPORT extern int yyparse(void);

/* Store client data */
static void *data;

/* All callback routines */
static struct {
  html_handle_error_fn error;
  html_handle_start_fn start;
  html_handle_end_fn end;
  html_handle_comment_fn comment;
  html_handle_text_fn text;
  html_handle_decl_fn decl;
  html_handle_pi_fn pi;
  html_handle_starttag_fn starttag;
  html_handle_emptytag_fn emptytag;
  html_handle_endtag_fn endtag;
  html_handle_endincl_fn endincl;
} handle = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/* Routines to bind concrete routines to the callbacks */
EXPORT void set_error_handler(html_handle_error_fn f) {handle.error = f;}
EXPORT void set_start_handler(html_handle_start_fn f) {handle.start = f;}
EXPORT void set_end_handler(html_handle_end_fn f) {handle.end = f;}
EXPORT void set_comment_handler(html_handle_comment_fn f) {handle.comment = f;}
EXPORT void set_text_handler(html_handle_text_fn f) {handle.text = f;}
EXPORT void set_decl_handler(html_handle_decl_fn f) {handle.decl = f;}
EXPORT void set_pi_handler(html_handle_pi_fn f) {handle.pi = f;}
EXPORT void set_starttag_handler(html_handle_starttag_fn f){handle.starttag=f;}
EXPORT void set_emptytag_handler(html_handle_emptytag_fn f){handle.emptytag=f;}
EXPORT void set_endtag_handler(html_handle_endtag_fn f) {handle.endtag = f;}
EXPORT void set_endincl_handler(html_handle_endincl_fn f) {handle.endincl = f;}

extern int yylex(void);
extern int yylineno;

static int nrerrors = 0;
#define MAX_ERRORS_REPORTED 20

/* yyerror -- report parse error */ 
static void yyerror(const string s)
{
  nrerrors++;
  if (nrerrors < MAX_ERRORS_REPORTED)
    handle.error(data, s, yylineno);
  else if (nrerrors == MAX_ERRORS_REPORTED)
    handle.error(data, "too many errors", yylineno);
  else
    ; /* don't report any more errors */
}

%}

%union {
    string s;
    pairlist p;
}

%token <s> TEXT COMMENT START END NAME STRING PROCINS
%token EMPTYEND DOCTYPE ENDINCL

%type <p> attribute attributes

%%

start
  :					{data = handle.start();}
    document				{handle.end(data);}
  ;
document
  : document COMMENT			{handle.comment(data, $2);}
  | document TEXT			{handle.text(data, $2);}
  | document starttag
  | document endtag
  | document decl
  | document PROCINS			{handle.pi(data, $2);}
  | document ENDINCL			{handle.endincl(data);}
  | document error
  | /* empty */
  ;
starttag
  : START attributes '>'		{handle.starttag(data,$1,$2);}
  | START attributes EMPTYEND		{handle.emptytag(data,$1,$2);}
  ;
attributes
  : attribute attributes		{$$ = $1; $$->next = $2;}
  | /* empty */				{$$ = NULL;}
  ;
attribute
  : NAME				{pairlist h = malloc(sizeof(*h));
					 assert(h != NULL); h->name = $1;
					 h->value=NULL; $$ = h;}
  | NAME '=' NAME			{pairlist h = malloc(sizeof(*h));
					 assert(h != NULL); h->name = $1;
					 h->value = $3; $$ = h;}
  | NAME '=' STRING			{pairlist h = malloc(sizeof(*h));
					 assert(h != NULL); h->name = $1;
					 h->value = $3; $$ = h;}
  ;
endtag
  : END '>'				{handle.endtag(data, $1);}
  ;
decl
  : DOCTYPE NAME NAME STRING STRING '>'	{handle.decl(data, $2, $4, $5);}
  | DOCTYPE NAME NAME STRING '>'	{if (strcasecmp($3, "public") == 0)
					   handle.decl(data, $2, $4, NULL);
					 else /* "system" */
					   handle.decl(data, $2, NULL, $4);}
  ;
