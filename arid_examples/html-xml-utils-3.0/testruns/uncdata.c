/*
 * uncdata -- remove CDATA sections from an XML file
 *
 * The input is scanned for occurrences of "<![CDATA[" and
 * corresponding "]]>". Those strings are removed and all occurrences
 * of "&", "<" and ">" in between them will be replaced by "&amp;",
 * "&;t;" and "&gt;" resp.
 *
 * The input must be 1 byte per character. If it is not, convert it to
 * UTF-8 first.
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 20 Feb 2002
 * Version: $Id: uncdata.c,v 1.3 2002/10/29 18:58:32 bbos Exp $
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>


/* process -- process one file */
static void process(FILE *f)
{
  int c;
  enum {INITIAL, START, CDATA1, CDATA2, CDATA3, CDATA4, CDATA5,
	CDATA6, CDATA7, CDATA98, CDATA99, CDATA, MARKUP, DECL1, DECL,
	COMMENT1, COMMENT, COMMENT99, DQUOTE, SQUOTE} state = INITIAL;

  /* No attempt at reporting errors for impossible XML,
     and no support for internal DTD subsets
  */
  while ((c = getc(f)) != EOF) {
    switch (state) {
    case INITIAL:
      if (c == '<') state = START;
      else putchar(c);
      break;
    case START:					/* Seen "<" */
      if (c == '!') state = DECL1;
      else if (c == '>') {putchar('<'); putchar('>'); state = INITIAL;}
      else {putchar('<'); putchar(c); state = MARKUP;}
      break;
    case MARKUP:				/* Inside "<...>" */
      if (c == '"') {putchar('"'); state = DQUOTE;}
      else if (c == '\'') {putchar('\''); state = SQUOTE;}
      else if (c == '>') {putchar('>'); state = INITIAL;}
      else putchar(c);
      break;
    case DQUOTE:				/* Inside double quotes */
      if (c == '"') {putchar('"'); state = MARKUP;}
      else putchar(c);
      break;
    case SQUOTE:				/* Inside single quotes */
      if (c == '\'') {putchar('\''); state = MARKUP;}
      else putchar(c);
      break;
    case DECL1:					/* Seen "<!" */
      if (c == '-') {printf("<!-"); state = COMMENT1;}
      else if (c == '[') state = CDATA1;
      else {putchar('<'); putchar('!'); putchar(c); state = DECL;}
      break;
    case DECL:					/* Inside "<!...>" */
      if (c == '-') {putchar('-'); state = COMMENT1;}
      else if (c == '>') {putchar('>'); state = INITIAL;}
      else putchar(c);
      break;
    case COMMENT1:				/* Seen "-" */
      if (c == '-') {putchar('-'); state = COMMENT;}
      else {putchar(c); state = DECL;}
      break;
    case COMMENT:				/* Seen "--" */
      if (c == '-') {putchar('-'); state = COMMENT99;}
      else putchar(c);
      break;
    case COMMENT99:				/* Seen "-" */
      if (c == '-') {putchar('-'); state = DECL;}
      else {putchar(c); state = COMMENT;}
      break;
    case CDATA1:				/* Seen "<![" */
      if (c == 'C') state = CDATA2;
      else {printf("<![%c", c); state = INITIAL;}
      break;
    case CDATA2:				/* Seen "<![C" */
      if (c == 'D') state = CDATA3;
      else {printf("<![C%c", c); state = INITIAL;}
      break;
    case CDATA3:				/* Seen "<![CD" */
      if (c == 'A') state = CDATA4;
      else {printf("<![CD%c", c); state = INITIAL;}
      break;
    case CDATA4:				/* Seen "<![CDA" */
      if (c == 'T') state = CDATA5;
      else {printf("<![CDA%c", c); state = INITIAL;}
      break;
    case CDATA5:				/* Seen "<![CDAT" */
      if (c == 'A') state = CDATA6;
      else {printf("<![CDAT%c", c); state = INITIAL;}
      break;
    case CDATA6:				/* Seen "<![CDATA" */
      if (c == '[') state = CDATA;
      else {printf("<![CDATA%c", c); state = INITIAL;}
      break;
    case CDATA:					/* Inside "<![CDATA[...]]>" */
      if (c == ']') state = CDATA98;
      else if (c == '<') puts("&lt;");
      else if (c == '>') puts("&gt;");
      else if (c == '&') puts("&amp;"); 
      else putchar(c);
      break;
    case CDATA98:				/* Seen "]" */
      if (c == ']') state = CDATA99;
      else {putchar(']'); putchar(c); state = CDATA;}
      break;
    case CDATA99:				/* Seen "]]" */
      if (c == '>') state = INITIAL;
      else {putchar(']'); putchar(']'); putchar(c); state = CDATA;}
      break;
    default:
      assert(!"Cannot happen!");
    }
  }
}

int main(int argc, char *argv[])
{
  int i, err = 0;
  FILE *f;

  if (argc == 1)
    process(stdin);
  else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)
    printf("Usage: %s [XML-FILE]...\n", argv[0]);
  else
    for (i = 1; i < argc; i++) {
      if (!(f = fopen(argv[i], "r"))) {perror(argv[i]); err++; continue;}
      process(f);
      if (fclose(f) != 0) {perror(argv[i]); err++; continue;}
    }
  return err;
}
