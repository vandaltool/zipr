/*
 * Clean up an HTML file:
 * Insert missing tags.
 *
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * 16 September 1997
 * Bert Bos
 * $Id: htmlclean.c,v 1.9 2003/01/21 19:26:03 bbos Exp $
 */
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include "export.h"
#include "types.e"
#include "tree.e"
#include "html.e"
#include "scan.e"

static Tree tree;

/* handle_error -- called when a parse error occurred */
void handle_error(void *clientdata, const string s, int lineno)
{
  fprintf(stderr, "%d: %s\n", lineno, s);
}

/* start -- called before the first event is reported */
void* start(void)
{
  tree = create();
  return NULL;
}
  
/* end -- called after the last even is reported */
void end(void *clientdata)
{
  /* skip */
}

/* handle_comment -- called after a comment is parsed */
void handle_comment(void *clientdata, string commenttext)
{
  tree = append_comment(tree, commenttext);
}

/* handle_text -- called after a tex chunk is parsed */
void handle_text(void *clientdata, string text)
{
  tree = append_text(tree, text);
}

/* handle_decl -- called after a declaration is parsed */
void handle_decl(void *clientdata, string gi,
		 string fpi, string url)
{
  tree = append_declaration(tree, gi, fpi, url);
}

/* handle_pi -- called after a PI is parsed */
void handle_pi(void *clientdata, string pi_text)
{
  tree = append_procins(tree, pi_text);
}

/* handle_starttag -- called after a start tag is parsed */
void handle_starttag(void *clientdata, string name, pairlist attribs)
{
  tree = html_push(tree, name, attribs);
}

/* handle_emptytag -- called after an empty tag is parsed */
void handle_emptytag(void *clientdata, string name, pairlist attribs)
{
  tree = html_push(tree, name, attribs);
}

/* handle_pop -- called after an endtag is parsed (name may be "") */
void handle_endtag(void *clientdata, string name)
{
  tree = html_pop(tree, name);
}


int main(int argc, char *argv[])
{
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

  if (argc == 1) {
    yyin = stdin;
  } else if (argc == 2) {
    yyin = fopen(argv[1], "r");
    if (yyin == NULL) {
      perror(argv[1]);
      exit(2);
    }
  } else {
    fprintf(stderr, "Version %s\n", VERSION);
    fprintf(stderr, "Usage: %s [html-file]\n", argv[0]);
    exit(1);
  }
  if (yyparse() != 0) {
    exit(3);
  }
  tree = get_root(tree);
  dumptree(tree);
  return 0;
  
}
