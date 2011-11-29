/* C code produced by gperf version 2.7 */
/* Command-line: gperf -a -c -C -o -t -p -T -k 1,2,$ -N lookup_element dtd.hash  */						/* -*-indented-text-*- */

/*
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 5 Nov 1998
 * $Id: dtd.hash,v 1.11 2003/01/21 19:48:46 bbos Exp $
 *
 * Input file for gperf, to generate a perfect hash function
 * for all HTML tags, and to store each element's type.
 *
 * mixed = element accepts text content
 * empty = element is empty
 * stag = start tag is required
 * etag = end tag is required
 * pre = element is preformatted
 * break_before, break_after = pretty-print with a newline before/after the elt
 * parents = array of possible parents, first one is preferred parent
 *
 * The DTD is strict HTML 4.0
 *
 */
#include <config.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "export.h"
#include "types.e"

#define MAXNAMELEN 10				/* Max. len. of elt. name */
EXPORTDEF(MAXNAMELEN)

EXPORT typedef struct _ElementType {
  char *name;
  Boolean mixed, empty, stag, etag, pre, break_before, break_after;
  char *parents[60];
} ElementType;

/* lookup_element -- look up the string in the hash table */
EXPORT const ElementType * lookup_element(register const char *str,
					  register unsigned int len);



#define TOTAL_KEYWORDS 78
#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 10
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 168
/* maximum key range = 168, duplicates = 0 */

#ifdef __GNUC__
__inline
#endif
static unsigned int
hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static const unsigned char asso_values[] =
    {
      169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169,   0, 169, 169,
      169, 169, 169, 169, 169, 169, 169, 169, 169,  20,
       25,  35,  40,  55,  30, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169,   0,  25,  70,
        5,  20,  45,  30,  10,   5, 169,  45,  15,  60,
       30,   0,  80,  10,  50,  30,   0,  15,   0, 169,
      169,  15, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169, 169, 169, 169, 169,
      169, 169, 169, 169, 169, 169
    };
  register int hval = len;

  switch (hval)
    {
      default:
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

#ifdef __GNUC__
__inline
#endif
const ElementType *
lookup_element (str, len)
     register const char *str;
     register unsigned int len;
{
  static const ElementType wordlist[] =
    {
      {""},
      {"a",		1, 0, 1, 1, 0, 0, 0, {"p", "abbr", "acronym", "address", "b", "bdo", "big", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {"tt",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {""}, {""}, {""}, {""},
      {"dt",		1, 0, 1, 0, 0, 1, 1, {"dl", NULL}},
      {""}, {""},
      {"%data",		1, 0, 1, 0, 0, 0, 0, {"p", NULL}},
      {"i",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {"td",		1, 0, 1, 0, 0, 1, 1, {"tr", NULL}},
      {"div",		1, 0, 1, 1, 0, 1, 1, {"body", "blockquote", "button", "dd", "del", "div", "fieldset", "form", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {""}, {""}, {""},
      {"dd",		1, 0, 1, 0, 0, 1, 1, {"dl", NULL}},
      {""}, {""},
      {"thead",		0, 0, 1, 0, 0, 1, 1, {"table", NULL}},
      {"q",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {"th",		1, 0, 1, 0, 0, 1, 1, {"tr", NULL}},
      {""}, {""},
      {"table",		0, 0, 1, 1, 0, 1, 1, {"body", "blockquote", "button", "dd", "del", "div", "fieldset", "form", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {""},
      {"li",		1, 0, 1, 0, 0, 1, 1, {"ul", "ol", NULL}},
      {"textarea",	1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {"html",		0, 0, 0, 0, 0, 1, 1, {NULL, NULL}},
      {"title",		1, 0, 1, 1, 0, 1, 1, {"head", NULL}},
      {"object",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "head", "i", "ins", "kbd", "label", "legend", "li", "object", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {"ol",		0, 0, 1, 1, 0, 1, 1, {"body", "blockquote", "button", "dd", "del", "div", "fieldset", "form", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {"bdo",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {""},
      {"label",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {""},
      {"dl",		0, 0, 1, 1, 0, 1, 1, {"body", "blockquote", "button", "dd", "del", "div", "fieldset", "form", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {"noscript",	0, 0, 1, 1, 0, 1, 1, {"body", "blockquote", "button", "dd", "del", "div", "fieldset", "form", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {"head",		0, 0, 0, 0, 0, 1, 1, {"html", NULL}},
      {"input",		0, 1, 1, 0, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {""},
      {"address",	1, 0, 1, 1, 0, 1, 1, {"body", "blockquote", "button", "dd", "del", "div", "fieldset", "form", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {"del",		1, 0, 1, 1, 0, 0, 0, {"p", "body", "a", "abbr", "acronym", "address", "b", "bdo", "big", "blockquote", "button", "caption", "cite", "code", "colgroup", "dd", "dfn", "div", "dl", "dt", "em", "fieldset", "form", "h1", "h2", "h3", "h4", "h5", "h6", "i", "kbd", "label", "legend", "li", "map", "noscript", "object", "ol", "optgroup", "option", "pre", "q", "samp", "select", "small", "span", "strong", "sub", "sup", "table", "tbody", "td", "textarea", "tfoot", "th", "thead", "tt", "ul", "var", NULL}},
      {"body",		0, 0, 0, 0, 0, 1, 1, {"html", NULL}},
      {"tbody",		0, 0, 0, 0, 0, 1, 1, {"table", NULL}},
      {"legend",		1, 0, 1, 1, 0, 1, 1, {"fieldset", NULL}},
      {"ul",		0, 0, 1, 1, 0, 1, 1, {"body", "blockquote", "button", "dd", "del", "div", "fieldset", "form", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {""},
      {"base",		0, 1, 1, 0, 0, 1, 1, {"head", NULL}},
      {"tfoot",		0, 0, 1, 0, 0, 1, 1, {"table", NULL}},
      {"b",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {"h1",		1, 0, 1, 1, 0, 1, 1, {"body", "blockquote", "button", "dd", "del", "div", "fieldset", "form", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {"var",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {"area",		0, 1, 1, 0, 0, 0, 0, {"map", NULL}},
      {"style",		1, 0, 1, 1, 1, 1, 1, {"head", NULL}},
      {"select",		0, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {""},
      {"fieldset",	1, 0, 1, 1, 0, 1, 1, {"body", "blockquote", "dd", "del", "div", "fieldset", "form", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {""}, {""}, {""},
      {"h2",		1, 0, 1, 1, 0, 1, 1, {"body", "blockquote", "button", "dd", "del", "div", "fieldset", "form", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {"big",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {""}, {""},
      {"strong",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {""},
      {"ins",		1, 0, 1, 1, 0, 0, 0, {"p", "body", "a", "abbr", "acronym", "address", "b", "bdo", "big", "blockquote", "button", "caption", "cite", "code", "colgroup", "dd", "dfn", "div", "dl", "dt", "em", "fieldset", "form", "h1", "h2", "h3", "h4", "h5", "h6", "i", "kbd", "label", "legend", "li", "map", "noscript", "object", "ol", "optgroup", "option", "pre", "q", "samp", "select", "small", "span", "strong", "sub", "sup", "table", "tbody", "td", "textarea", "tfoot", "th", "thead", "tt", "ul", "var", NULL}},
      {"link",		0, 1, 1, 0, 0, 1, 1, {"head", NULL}},
      {"blockquote",	0, 0, 1, 1, 0, 1, 1, {"body", "blockquote", "button", "dd", "del", "div", "fieldset", "form", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {""},
      {"h6",		1, 0, 1, 1, 0, 1, 1, {"body", "blockquote", "button", "dd", "del", "div", "fieldset", "form", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {"sub",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {""}, {""},
      {"button",		1, 0, 1, 1, 0, 1, 1, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {""},
      {"kbd",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {"abbr",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {""}, {""},
      {"h3",		1, 0, 1, 1, 0, 1, 1, {"body", "blockquote", "button", "dd", "del", "div", "fieldset", "form", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {"dfn",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {"meta",		0, 1, 1, 0, 0, 1, 1, {"head", NULL}},
      {""}, {""}, {""},
      {"col",		0, 1, 1, 0, 0, 0, 0, {"colgroup", "table", NULL}},
      {""}, {""}, {""},
      {"h4",		1, 0, 1, 1, 0, 1, 1, {"body", "blockquote", "button", "dd", "del", "div", "fieldset", "form", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {""},
      {"code",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {""}, {""}, {""},
      {"img",		0, 1, 1, 0, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {"cite",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {""}, {""},
      {"tr",		0, 0, 1, 0, 0, 1, 1, {"tbody", "tfoot", "thead", NULL}},
      {""}, {""}, {""},
      {"script",		1, 0, 1, 1, 1, 0, 0, {"p", "body", "a", "abbr", "acronym", "address", "b", "bdo", "big", "blockquote", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "form", "h1", "h2", "h3", "h4", "h5", "h6", "head", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {"caption",	1, 0, 1, 1, 0, 1, 1, {"table", NULL}},
      {""},
      {"form",		0, 0, 1, 1, 0, 1, 1, {"body", "blockquote", "dd", "del", "div", "fieldset", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {"small",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {""},
      {"hr",		0, 1, 1, 0, 0, 1, 1, {"body", "blockquote", "button", "dd", "del", "div", "fieldset", "form", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {""},
      {"samp",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {""},
      {"option",		1, 0, 1, 0, 0, 1, 1, {"select", "optgroup", NULL}},
      {""}, {""}, {""}, {""}, {""},
      {"h5",		1, 0, 1, 1, 0, 1, 1, {"body", "blockquote", "button", "dd", "del", "div", "fieldset", "form", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {""}, {""}, {""}, {""},
      {"br",		0, 1, 1, 0, 0, 0, 1, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {"sup",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"acronym",	1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {""}, {""}, {""}, {""},
      {"em",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {"map",		0, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {"span",		1, 0, 1, 1, 0, 0, 0, {"p", "a", "abbr", "acronym", "address", "b", "bdo", "big", "button", "caption", "cite", "code", "dd", "del", "dfn", "div", "dt", "em", "fieldset", "h1", "h2", "h3", "h4", "h5", "h6", "i", "ins", "kbd", "label", "legend", "li", "object", "pre", "q", "samp", "small", "span", "strong", "sub", "sup", "td", "th", "tt", "var", NULL}},
      {"param",		0, 1, 1, 0, 0, 1, 1, {"object", NULL}},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"pre",		1, 0, 1, 1, 1, 1, 1, {"body", "blockquote", "button", "dd", "del", "div", "fieldset", "form", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {""}, {""}, {""}, {""},
      {"colgroup",	0, 0, 1, 1, 0, 1, 1, {"table", NULL}},
      {""}, {""},
      {"p",		1, 0, 1, 0, 0, 1, 1, {"body", "blockquote", "button", "dd", "del", "div", "fieldset", "form", "ins", "li", "map", "noscript", "object", "td", "th", NULL}},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"optgroup",	0, 0, 1, 1, 0, 1, 1, {"select", NULL}}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].name;

          if (*str == *s && !strncmp (str + 1, s + 1, len - 1))
            return &wordlist[key];
        }
    }
  return 0;
}
