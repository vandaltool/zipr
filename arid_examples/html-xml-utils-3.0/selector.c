/*
 * Type definitions and a parser for CSS selectors.
 *
 * Only parses selectors that allow incremental rendering
 * of a document.
 *
 * The Selector type is a linked list of simple selectors, with the
 * subject at the head, and its context linked from the "context"
 * field. The "combinator" field is the relation between this simple
 * selector and its context.
 *
 * To do: backslash escapes elsewhere than in element names.
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 8 July 2001
 * Version: $Id: selector.c,v 1.6 2003/01/21 19:44:51 bbos Exp $
 **/

#include <config.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#ifdef HAVE_STRING_H
#  include <string.h>
#elif HAVE_STRINGS_H
#  include <strings.h>
#endif
#include "export.h"
#include "heap.e"
#include "types.e"
#include "errexit.e"

EXPORT typedef enum {				/* Pseudo-classes */
  Root, NthChild, NthOfType, FirstChild, FirstOfType, Lang
} PseudoType;

EXPORT typedef struct _PseudoCond {
  PseudoType type;
  int a, b;					/* :nth-child(an+b) */
  string s;					/* :lang(s) */
  struct _PseudoCond *next;
} PseudoCond;

EXPORT typedef enum {				/* =, ~=, ^=, $= *= |= */
  Exists, Equals, Includes, StartsWith, EndsWidth, Contains, LangMatch,
  HasClass, HasID				/* ".foo", "#foo" */
} Operator;

EXPORT typedef struct _AttribCond {
  Operator op;
  string name;					/* If not HasClass/ID */
  string value;					/* If op!=Exists */
  struct _AttribCond *next;
} AttribCond;

EXPORT typedef enum {
  Descendant, Child, Adjacent, Sibling
} Combinator;

EXPORT typedef struct _SimpleSelector {
  string name;					/* NULL is "*" */
  AttribCond *attribs;
  PseudoCond *pseudos;
  Combinator combinator;			/* If context not NULL */
  struct _SimpleSelector *context;
} SimpleSelector, *Selector;

typedef enum {
  INIT, SLASH, START_SIMPLE, START_CLASS, START_ID, COMMENT,
  AFTER_SIMPLE, COMMENT_STAR, START_ATTR, START_PSEUDO, ESC0,
  TYPE, AFTER_TYPE, ESCAPE, CLASS, ID, ATTR, AFTER_ATTR, EQ,
  START_VALUE, DSTRING, SSTRING, VALUE, HASH, AFTER_VALUE, PSEUDO_R,
  PSEUDO_RO, PSEUDO_ROO, PSEUDO_ROOT, PSEUDO_F, PSEUDO_FI, PSEUDO_FIR,
  PSEUDO_FIRS, PSEUDO_FIRST, PSEUDO_FIRST_, PSEUDO_FIRST_C,
  PSEUDO_FIRST_CH, PSEUDO_FIRST_CHI, PSEUDO_FIRST_CHIL,
  PSEUDO_FIRST_CHILD, PSEUDO_FIRST_CHILD_, PSEUDO_N, PSEUDO_NT,
  PSEUDO_NTH, PSEUDO_NTH_, PSEUDO_NTH_C, PSEUDO_NTH_CH,
  PSEUDO_NTH_CHI, PSEUDO_NTH_CHIL, PSEUDO_NTH_CHILD, PSEUDO_NTH_CHILD_,
  PSEUDO_L, PSEUDO_LA, PSEUDO_NTH_LAN, PSEUDO_NTH_LANG, PSEUDO_NTH_LANG_,
  PSEUDO_NTH_O, PSEUDO_NTH_OF, PSEUDO_NTH_OF_, PSEUDO_NTH_OF_T,
  PSEUDO_NTH_OF_TY, PSEUDO_NTH_OF_TYP, PSEUDO_NTH_OF_TYPE,
  PSEUDO_NTH_OF_TYPE_, PSEUDO_FIRST_O, PSEUDO_FIRST_OF,
  PSEUDO_FIRST_OF_, PSEUDO_FIRST_OF_T, PSEUDO_FIRST_OF_TY,
  PSEUDO_FIRST_OF_TYP, PSEUDO_FIRST_OF_TYPE, END_PSEUDO,
  START_INT, INT, PSEUDO_LAN, PSEUDO_LANG, PSEUDO_LANG_, LANG,
  AFTER_MUL, AFTER_MUL_N, AFTER_MUL_NPLUS, PSEUDO__O,
  PSEUDO__OD, PSEUDO__ODD, PSEUDO__E,
  PSEUDO__EV, PSEUDO__EVE, PSEUDO__EVEN, PSEUDO__MINUS, PSEUDO__NEG
  
} State;


/* strappc -- append a character to a malloc'ed string */
static void strappc(string *s, unsigned char c)
{
  int len = strlen(*s);
  renewarray(*s, len + 2);
  (*s)[len] = c;
  (*s)[len+1] = '\0';
}

/* pseudos_to_string -- convert pseudo-class selectors to a string */
static string pseudos_to_string(const PseudoCond *p)
{
  string h, s = newstring("");
  unsigned char t1[30], t2[30];

  sprintf(t1, "%d", p->a);
  sprintf(t2, "%d", p->b);
  switch (p->type) {
  case Root:
    strapp(&s, ":root", NULL); break;
  case NthChild:
    strapp(&s, ":nth-child(", t1, "n+", t2, ")", NULL); break;
  case NthOfType:
    strapp(&s, ":nth-of-type(", t1, "n+", t2, ")", NULL); break;
  case Lang:
    strapp(&s, ":lang(", p->s, ")", NULL); break;
  case FirstChild:
    strapp(&s, ":first-child", NULL); break;
  case FirstOfType:
    strapp(&s, ":first-of-type", NULL); break;
  default:
    assert(!"Cannot happen");
  }
  if (p->next) {
    strapp(&s, (h = pseudos_to_string(p->next)), NULL);
    dispose(h);
  }
  return s;
}

/* attribs_to_string -- convert attribute selectors to a string */
static string attribs_to_string(const AttribCond *a)
{
  string h, s = newstring("");

  /* To do: escape illegal characters */
  switch (a->op) {
  case HasClass:
    strapp(&s, ".", a->value, NULL); break;
  case HasID:
    strapp(&s, "#", a->value, NULL); break;
  case Exists:
    strapp(&s, "[", a->name, "]", NULL); break;
  case Equals:
    strapp(&s, "[", a->name, "=\"",  a->value, "\"]", NULL); break;
  case Includes:
    strapp(&s, "[", a->name, "~=\"", a->value, "\"]", NULL); break;
  case StartsWith:
    strapp(&s, "[", a->name, "^=\"", a->value, "\"]", NULL); break;
  case EndsWidth:
    strapp(&s, "[", a->name, "$=\"", a->value, "\"]", NULL); break;
  case LangMatch:
    strapp(&s, "[", a->name, "|=\"", a->value, "\"]", NULL); break;
  case Contains:
    strapp(&s, "[", a->name, "*=\"", a->value, "\"]", NULL); break;
  default:
    assert(!"Cannot happen");
  }
  if (a->next) {
    strapp(&s, (h = attribs_to_string(a->next)), NULL);
    dispose(h);
  }
  return s;
}

/* selector_to_string -- convert selector back to a string */
EXPORT string selector_to_string(const Selector selector)
{
  string h, s = newstring("");

  strapp(&s, selector->name ? selector->name : (string)"*", NULL);
  if (selector->attribs) {
    h = attribs_to_string(selector->attribs);
    strapp(&s, h, NULL);
    dispose(h);
  }
  if (selector->pseudos) {
    h = pseudos_to_string(selector->pseudos);
    strapp(&s, h, NULL);
    dispose(h);
  }
  if (selector->context) {
    h = s;
    s = selector_to_string(selector->context);
    switch (selector->combinator) {
    case Descendant: strapp(&s, " ", h, NULL); break;
    case Child:      strapp(&s, " > ", h, NULL); break;
    case Adjacent:   strapp(&s, " + ", h, NULL); break;
    case Sibling:    strapp(&s, " ~ ", h, NULL); break;
    default: assert(!"Cannot happen");
    }
    dispose(h);
  }
  return s;
}

/* push_sel -- allocate memory for a new selector; initialize */
static void push_sel(Selector *selector, Combinator combinator)
{
  Selector h;

  new(h);
  h->name = NULL;
  h->attribs = NULL;
  h->pseudos = NULL;
  h->context = *selector;
  h->combinator = combinator;
  *selector = h;
}

/* isnmstart -- check if a character can start an identifier */
static Boolean isnmstart(unsigned int c)
{
  return ('a' <= c && c <= 'z')
    || ('A' <= c && c <= 'Z')
    || (c == '_')
    || (c >= '\200');
}

/* isnmchar -- check if a character can be inside an identifier */
static Boolean isnmchar(unsigned int c)
{
  return ('a' <= c && c <= 'z')
    || ('A' <= c && c <= 'Z')
    || ('0' <= c && c <= '9')
    || (c == '_')
    || (c == '-')
    || (c >= '\200');
}

/* parse_selector -- parse the selector in s */
EXPORT Selector parse_selector(const string selector)
{
  State saved_state, state = INIT;
  string s = selector;
  AttribCond *attsel;
  PseudoCond *pseudosel;
  Selector sel = NULL;
  int n, esc;

  /* To do: pseudos should be case-insensitive */

  push_sel(&sel, Descendant);

  while (*s) {
    switch (state) {
    case INIT:					/* Expect a simple sel */
      if (isspace(*s)) s++;
      else if (*s == '/') {s++; saved_state = INIT; state = SLASH;}
      else state = START_SIMPLE;
      break;
    case AFTER_SIMPLE:				/* Expect a combinator */
      if (isspace(*s)) {s++;}
      else if (*s == '/') {s++; saved_state = AFTER_SIMPLE; state=SLASH;}
      else if (*s == '+') {s++; push_sel(&sel, Adjacent); state = INIT;}
      else if (*s == '>') {s++; push_sel(&sel, Child); state = INIT;}
      else if (*s == '~') {s++; push_sel(&sel, Sibling); state = INIT;}
      else {push_sel(&sel, Descendant); state = START_SIMPLE;}
      break;
    case SLASH:					/* Expect a '*'  */
      if (*s == '*') {s++; state = COMMENT;}
      else errexit("Syntax error in selector at '/'\n");
      break;
    case COMMENT:				/* Inside comment */
      if (*s == '*') state = COMMENT_STAR;
      s++;
      break;
    case COMMENT_STAR:				/* Maybe end comment */
      if (*s == '/') state = saved_state;
      else if (*s != '*') state = COMMENT;
      s++;
      break;
    case START_SIMPLE:				/* Start simple sel */
      if (*s == '*') {s++; state = AFTER_TYPE;}	/* Universal selector */
      else if (*s == '.') {s++; state = START_CLASS;}
      else if (*s == '#') {s++; state = START_ID;}
      else if (*s == '[') {s++; state = START_ATTR;}
      else if (*s == ':') {s++; state = START_PSEUDO;}
      else if (*s == '\\') {sel->name = newstring(""); s++; state=ESC0;}
      else if (isnmstart(*s)) {sel->name = newstring(""); state = TYPE;}
      else  errexit("Syntax error at \"%c\"\n", *s);
      break;
    case TYPE:					/* Type selector */
      if (*s == '\\') {s++; state = ESC0;}
      else if (isnmchar(*s)) {strappc(&sel->name, *s); s++;}
      else state = AFTER_TYPE;
      break;
    case ESC0:				/* Just seen a '\' */
      if (isxdigit(*s)) {esc = 0; state = ESCAPE;}
      else {strappc(&sel->name, *s); s++; state = TYPE;}
      break;
    case ESCAPE:				/* Hex escape */
      if ('a' <= *s && *s <= 'f') {esc = 16 * esc + *s - 'a' + 10; s++;}
      else if ('A'<=*s && *s<='F') {esc = 16 * esc + *s - 'A' + 10; s++;}
      else if ('0'<=*s && *s<='9') {esc = 16 * esc + *s - '0'; s++;}
      else if (isspace(*s)) {strappc(&sel->name, esc); s++; state=TYPE;}
      else {strappc(&sel->name, esc); state = TYPE;}
      break;
    case AFTER_TYPE:				/* After a type sel */
      if (*s == '.') {s++; state = START_CLASS;}
      else if (*s == '#') {s++; state = START_ID;}
      else if (*s == '[') {s++; state = START_ATTR;}
      else if (*s == ':') {s++; state = START_PSEUDO;}
      else state = AFTER_SIMPLE;
      break;
    case START_CLASS:				/* Just seen a '.' */
      if (isnmstart(*s)) {
	new(attsel);
	attsel->op = HasClass;
	attsel->value = newstring("");
	attsel->next = sel->attribs;
	sel->attribs = attsel;
	strappc(&sel->attribs->value, *s);
	s++;
	state = CLASS;
      } else errexit("Expected letter instead of \"%c\" after \".\"",*s);
      break;
    case CLASS:					/* Inside class name */
      if (isnmchar(*s)) {strappc(&sel->attribs->value, *s); s++;}
      else state = AFTER_TYPE;
      break;
    case START_ID:				/* Just seen a '#' */
      if (isnmchar(*s)) {
	new(attsel);
	attsel->op = HasID;
	attsel->value = newstring("");
	attsel->next = sel->attribs;
	sel->attribs = attsel;
	strappc(&sel->attribs->value, *s);
	s++;
	state = ID;
      } else errexit("Expected letter instead of \"%c\" after \".\"",*s);
      break;
    case ID:					/* Inside name of ID */
      if (isnmchar(*s)) {strappc(&sel->attribs->value, *s); s++;}
      else state = AFTER_TYPE;
      break;
    case START_ATTR:				/* Just seen a '[' */
      if (isspace(*s)) s++;
      else if (*s == '/') {saved_state = START_ATTR; state = SLASH; s++;}
      else if (isnmstart(*s)) {
	new(attsel);
	attsel->name = newstring("");
	attsel->next = sel->attribs;
	sel->attribs = attsel;
	strappc(&sel->attribs->name, *s);
	s++;
	state = ATTR;
      } else errexit("Expected letter instead of \"%c\" after \"[\"",*s);
      break;
    case ATTR:					/* Inside attrib name */
      if (isnmchar(*s)) {strappc(&sel->attribs->name, *s); s++;}
      else state = AFTER_ATTR;
      break;
    case AFTER_ATTR:
      if (isspace(*s)) s++;
      else if (*s == '/') {s++; saved_state = AFTER_ATTR; state = SLASH;}
      else if (*s == ']') {s++; sel->attribs->op = Exists; state = AFTER_TYPE;}
      else if (*s == '~') {s++; sel->attribs->op = Includes; state = EQ;}
      else if (*s == '|') {s++; sel->attribs->op = LangMatch; state=EQ;}
      else if (*s == '^') {s++; sel->attribs->op = StartsWith; state=EQ;}
      else if (*s == '$') {s++; sel->attribs->op = EndsWidth; state=EQ;}
      else if (*s == '*') {s++; sel->attribs->op = Contains; state = EQ;}
      else {sel->attribs->op = Equals; state = EQ;}
      break;
    case EQ:					/* Expect '=' */
      if (*s != '=') errexit("Expected '=' instead of \"%c\"", *s);
      else {s++; sel->attribs->value = newstring(""); state=START_VALUE;}
      break;
    case START_VALUE:				/* After '=' */
      if (isspace(*s)) s++;
      else if (*s == '/') {s++; saved_state = START_VALUE; state=SLASH;}
      else if (*s == '"') {s++; state = DSTRING;}
      else if (*s == '\'') {s++; state = SSTRING;}
      else if (!isnmstart(*s)) errexit("Syntax error at \"%c\"\n", *s);
      else {strappc(&sel->attribs->value, *s); s++; state = VALUE;}
      break;
    case DSTRING:				/* Inside "..." */
      if (*s == '"') {s++; state = AFTER_VALUE;}
      else {strappc(&sel->attribs->value, *s); s++;}
      break;
    case SSTRING:				/* Inside "..." */
      if (*s == '\'') {s++; state = AFTER_VALUE;}
      else {strappc(&sel->attribs->value, *s); s++;}
      break;
    case VALUE:					/* Inside keyword */
      if (isnmchar(*s)) {strappc(&sel->attribs->value, *s); s++;}
      else state = AFTER_VALUE;
      break;
    case AFTER_VALUE:				/* Expect ']' */
      if (isspace(*s)) s++;
      else if (*s == '/') {s++; saved_state = AFTER_VALUE; state = HASH;}
      else if (*s == ']') {s++; state = AFTER_TYPE;}
      else errexit("Expected ']' instead of \"%c\"\n", *s);
      break;
    case START_PSEUDO:				/* After ':' */
      new(pseudosel);
      pseudosel->next = sel->pseudos;
      sel->pseudos = pseudosel;
      if (*s == 'r' || *s == 'R') {s++; state = PSEUDO_R;}
      else if (*s == 'n' || *s == 'N') {s++; state = PSEUDO_N;}
      else if (*s == 'f' || *s == 'F') {s++; state = PSEUDO_F;}
      else if (*s == 'l' || *s == 'L') {s++; state = PSEUDO_L;}
      else errexit("Unknown pseudo-class \":%c...\"\n", *s);
      break;
    case PSEUDO_R:				/* After ':r' */
      if (*s == 'o' || *s == 'O') {s++; state = PSEUDO_RO;}
      else errexit("Unknown pseudo-class \":r%c...\"\n", *s);
      break;
    case PSEUDO_RO:				/* After ':ro' */
      if (*s == 'o' || *s == 'O') {s++; state = PSEUDO_ROO;}
      else errexit("Unknown pseudo-class \":ro%c...\"\n", *s);
      break;
    case PSEUDO_ROO:				/* After ':roo' */
      if (*s == 't' || *s == 'T') {sel->pseudos->type = Root; s++; state=PSEUDO_ROOT;}
      else errexit("Unknown pseudo-class \":roo%c...\"\n", *s);
      break;
    case PSEUDO_ROOT:				/* After ':root' */
      if (!isnmchar(*s)) state = AFTER_TYPE;
      else errexit("Unknown pseudo-class \":root%c...\"\n", *s);
      break;
    case PSEUDO_N:				/* After ':n' */
      if (*s == 't' || *s == 'T') {s++; state = PSEUDO_NT;}
      else errexit("Unknown pseudo-class \":n%c...\"\n", *s);
      break;
    case PSEUDO_NT:				/* After ':nt' */
      if (*s == 'h' || *s == 'H') {s++; state = PSEUDO_NTH;}
      else errexit("Unknown pseudo-class \":nt%c...\"\n", *s);
      break;
    case PSEUDO_NTH:				/* After ':nth' */
      if (*s == '-') {s++; state = PSEUDO_NTH_;}
      else errexit("Unknown pseudo-class \":nth%c...\"\n", *s);
      break;
    case PSEUDO_NTH_:				/* After ':nth_' */
      if (*s == 'c' || *s == 'C') {s++; state = PSEUDO_NTH_C;}
      else if (*s == 'o' || *s == 'O') {s++; state = PSEUDO_NTH_O;}
      else errexit("Unknown pseudo-class \":nth_%c...\"\n", *s);
      break;
    case PSEUDO_NTH_C:				/* After ':nth_c' */
      if (*s == 'h' || *s == 'H') {s++; state = PSEUDO_NTH_CH;}
      else errexit("Unknown pseudo-class \":nth_c%c...\"\n", *s);
      break;
    case PSEUDO_NTH_CH:				/* After ':nth_ch' */
      if (*s == 'i' || *s == 'I') {s++; state = PSEUDO_NTH_CHI;}
      else errexit("Unknown pseudo-class \":nth_ch%c...\"\n", *s);
      break;
    case PSEUDO_NTH_CHI:			/* After ':nth_chi' */
      if (*s == 'l' || *s == 'L') {s++; state = PSEUDO_NTH_CHIL;}
      else errexit("Unknown pseudo-class \":nth_chi%c...\"\n", *s);
      break;
    case PSEUDO_NTH_CHIL:			/* After ':nth_chil' */
      if (*s == 'd' || *s == 'D') {s++; state = PSEUDO_NTH_CHILD;}
      else errexit("Unknown pseudo-class \":nth_chil%c...\"\n", *s);
      break;
    case PSEUDO_NTH_CHILD:			/* After ':nth_child' */
      if (*s == '(') {s++; state = PSEUDO_NTH_CHILD_;}
      else errexit("Unknown pseudo-class \":nth_child%c...\"\n", *s);
      break;
    case PSEUDO_NTH_CHILD_:			/* After ':nth_child(' */
      sel->pseudos->type = NthChild; sel->pseudos->a = 0;
      if (isspace(*s)) s++;
      else if (*s == '/') {s++; saved_state = state; state = SLASH;}
      else if (isdigit(*s)) {saved_state = AFTER_MUL; state = START_INT;}
      else if (*s == '-') {s++; state = PSEUDO__MINUS;}
      else if (*s == 'o' || *s == 'O') {s++; state = PSEUDO__O;}
      else if (*s == 'e' || *s == 'E') {s++; state = PSEUDO__E;}
      else if (*s == 'n' || *s == 'N') {s++; sel->pseudos->a = 1;
        state=AFTER_MUL_N;}
      else errexit("Expected digit after \"nth-child(\" at \"%c\"\n",*s);
      break;
    case PSEUDO__MINUS:				/* After :nth-...(- */
      if (isspace(*s)) s++;
      else if (*s == '/') {s++; saved_state = state; state = SLASH;}
      else if (isdigit(*s)) {saved_state = PSEUDO__NEG; state=START_INT;}
      else if (*s == 'n' || *s == 'N') {s++; sel->pseudos->a = -1;
        state=AFTER_MUL_N;}
      else errexit("Expected digit after \":nth-...(\" at \"%c\"\n",*s);
      break;
    case PSEUDO__NEG:				/* After -<num> */
      sel->pseudos->b = -sel->pseudos->b; state = AFTER_MUL;
      break;
    case PSEUDO__O:				/* After :nth...(o */
      if (*s == 'd' || *s == 'D') {s++; state = PSEUDO__OD;}
      else errexit("Illegal character \"%c\" in \":nth-...(\"\n", *s);
      break;
    case PSEUDO__OD:				/* After :nth...(od */
      if (*s == 'd' || *s == 'D') {s++; state = PSEUDO__ODD;}
      else errexit("Illegal character \"%c\" in \":nth-...(\"\n", *s);
      break;
    case PSEUDO__ODD:				/* After :nth-...(odd */
      if (!isnmchar(*s)) {state = END_PSEUDO;
        sel->pseudos->a = 2; sel->pseudos->b = 1;}
      else errexit("Illegal character \"%c\" in \":nth-...(\"\n", *s);
      break;
    case PSEUDO__E:				/* After :nth-...(e */
      if (*s == 'v' || *s == 'V') {s++; state = PSEUDO__EV;}
      else errexit("Illegal character \"%c\" in \":nth-...(\"\n", *s);
      break;
    case PSEUDO__EV:				/* After :nth-...(ev */
      if (*s == 'e' || *s == 'E') {s++; state = PSEUDO__EVE;}
      else errexit("Illegal character \"%c\" in \":nth-...(\"\n", *s);
      break;
    case PSEUDO__EVE:				/* After :nth-...(eve */
      if (*s == 'n' || *s == 'N') {s++; state = PSEUDO__EVEN;}
      else errexit("Illegal character \"%c\" in \":nth-...(\"\n", *s);
      break;
    case PSEUDO__EVEN:				/* Afte :nth-...(even */
      if (!isnmchar(*s)) {state = END_PSEUDO;
        sel->pseudos->a = 2; sel->pseudos->b = 0;}
      else errexit("Illegal character \"%c\" in \":nth-...(\"\n", *s);
      break;
    case START_INT:				/* At a digit */
      n = *s - '0'; s++; state = INT;
      break;
    case INT:					/* Inside number */
      if (isdigit(*s)) {n = 10 * n + *s - '0'; s++;}
      else {sel->pseudos->b = n; state = saved_state;}
      break;
    case AFTER_MUL:				/* After "...(<num>" */
      if (isspace(*s)) s++;
      else if (*s == '/') {s++; saved_state = state; state = SLASH;}
      else if (*s == ')') {s++; state = AFTER_TYPE;}
      else if (*s == 'n' || *s == 'N') {s++; state = AFTER_MUL_N;
        sel->pseudos->a = sel->pseudos->b; sel->pseudos->b = 0;}
      else errexit("Illegal character \"%c\"\n", *s);
      break;
    case AFTER_MUL_N:				/* After "...(<num>N" */
      if (isspace(*s)) s++;
      else if (*s == '/') {s++; saved_state = state; state = SLASH;}
      else if (*s == '+') {s++; state = AFTER_MUL_NPLUS;}
      else if (*s == ')') {s++; state = AFTER_TYPE;}
      else errexit("Illegal character \"%c\"\n", *s);
      break;
    case AFTER_MUL_NPLUS:			/* After "...(<num>N+" */
      if (isspace(*s)) s++;
      else if (*s == '/') {s++; saved_state = state; state = SLASH;}
      else if (isdigit(*s)) {saved_state = END_PSEUDO; state=START_INT;}
      else errexit("Expected a digit after \"+\" at \"%c\"\n", *s);
      break;
    case END_PSEUDO:				/* Expect ')' */
      if (isspace(*s)) s++;
      else if (*s == '/') {s++; saved_state = END_PSEUDO; state = SLASH;}
      else if (*s == ')') {s++; state = AFTER_TYPE;}
      else errexit("Illegal character \"%c\"\n", *s);
      break;
    case PSEUDO_NTH_O:				/* After ':nth_o' */
      if (*s == 'f' || *s == 'F') {s++; state = PSEUDO_NTH_OF;}
      else errexit("Unknown pseudo-class \":nth_o%c...\"", *s);
      break;
    case PSEUDO_NTH_OF:				/* After ':nth_of' */
      if (*s == '-') {s++; state = PSEUDO_NTH_OF_;}
      else errexit("Unknown pseudo-class \":nth_of%c...\"", *s);
      break;
    case PSEUDO_NTH_OF_:			/* After ':nth_of_' */
      if (*s == 't' || *s == 'T') {s++; state = PSEUDO_NTH_OF_T;}
      else errexit("Unknown pseudo-class \":nth_of_%c...\"", *s);
      break;
    case PSEUDO_NTH_OF_T:			/* After ':nth_of_t' */
      if (*s == 'y' || *s == 'Y') {s++; state = PSEUDO_NTH_OF_TY;}
      else errexit("Unknown pseudo-class \":nth_of_t%c...\"", *s);
      break;
    case PSEUDO_NTH_OF_TY:			/* After ':nth_of_ty' */
      if (*s == 'p' || *s == 'P') {s++; state = PSEUDO_NTH_OF_TYP;}
      else errexit("Unknown pseudo-class \":nth_of_ty%c...\"", *s);
      break;
    case PSEUDO_NTH_OF_TYP:			/* After ':nth_of_typ' */
      if (*s == 'e' || *s == 'E') {s++; state = PSEUDO_NTH_OF_TYPE;}
      else errexit("Unknown pseudo-class \":nth_of_typ%c...\"", *s);
      break;
    case PSEUDO_NTH_OF_TYPE:			/* After :nth_of_type */
      if (*s == '(') {s++; state = PSEUDO_NTH_OF_TYPE_;}
      else errexit("Unknown pseudo-class \":nth_of_type%c...\"", *s);
      break;
    case PSEUDO_NTH_OF_TYPE_:			/* After :nth_of_type( */
      sel->pseudos->type = NthChild; sel->pseudos->a = 0;
      if (isspace(*s)) s++;
      else if (*s == '/') {s++; saved_state = state; state = SLASH;}
      else if (isdigit(*s)) {saved_state = AFTER_MUL; state = START_INT;}
      else if (*s == '-') {s++; state = PSEUDO__MINUS;}
      else if (*s == 'o' || *s=='O') {s++; state = PSEUDO__O;}
      else if (*s == 'e' || *s=='E') {s++; state = PSEUDO__E;}
      else if (*s == 'n' || *s=='N') {s++; sel->pseudos->a=1;state=AFTER_MUL_N;}
      else errexit("Expected digit after \":nth-of_type(\" at \"%c\"\n",*s);
      break;
    case PSEUDO_F:				/* After ':f' */
      if (*s == 'i' || *s == 'I') {s++; state = PSEUDO_FI;}
      else errexit("Unknown pseudo-class \":f%c...\"", *s);
      break;
    case PSEUDO_FI:				/* After ':fi' */
      if (*s == 'r' || *s == 'R') {s++; state = PSEUDO_FIR;}
      else errexit("Unknown pseudo-class \":fi%c...\"", *s);
      break;
    case PSEUDO_FIR:				/* After ':fir' */
      if (*s == 's' || *s == 'S') {s++; state = PSEUDO_FIRS;}
      else errexit("Unknown pseudo-class \":fir%c...\"", *s);
      break;
    case PSEUDO_FIRS:				/* After ':firs' */
      if (*s == 't' || *s == 'T') {s++; state = PSEUDO_FIRST;}
      else errexit("Unknown pseudo-class \":firs%c...\"", *s);
      break;
    case PSEUDO_FIRST:				/* After ':first' */
      if (*s == '-') {s++; state = PSEUDO_FIRST_;}
      else errexit("Unknown pseudo-class \":first%c...\"", *s);
      break;
    case PSEUDO_FIRST_:				/* After ':first_' */
      if (*s == 'c' || *s == 'C') {s++; state = PSEUDO_FIRST_C;}
      else if (*s == 'o') {s++; state = PSEUDO_FIRST_O;}
      else errexit("Unknown pseudo-class \":first_%c...\"", *s);
      break;
    case PSEUDO_FIRST_C:			/* After ':first_c' */
      if (*s == 'h' || *s == 'H') {s++; state = PSEUDO_FIRST_CH;}
      else errexit("Unknown pseudo-class \":first_c%c...\"", *s);
      break;
    case PSEUDO_FIRST_CH:			/* After ':first_ch' */
      if (*s == 'i' || *s == 'I') {s++; state = PSEUDO_FIRST_CHI;}
      else errexit("Unknown pseudo-class \":first_ch%c...\"", *s);
      break;
    case PSEUDO_FIRST_CHI:			/* After ':first_chi' */
      if (*s == 'l' || *s == 'L') {s++; state = PSEUDO_FIRST_CHIL;}
      else errexit("Unknown pseudo-class \":first_chi%c...\"", *s);
      break;
    case PSEUDO_FIRST_CHIL:			/* After ':first_chil' */
      if (*s != 'd' && *s != 'D') errexit("Unknown pseudo-class \":first_chil%c...\"", *s);
      else {sel->pseudos->type = FirstChild; s++; state = PSEUDO_FIRST_CHILD;}
      break;
    case PSEUDO_FIRST_CHILD:			/* After ':first_child' */
      if (!isnmchar(*s)) {state = AFTER_TYPE;}
      else errexit("Unknown pseudo-class \":first_child%c...\"", *s);
      break;
    case PSEUDO_FIRST_O:			/* After ':first_o' */
      if (*s == 'f' || *s == 'F') {s++; state = PSEUDO_FIRST_OF;}
      else errexit("Unknown pseudo-class \":first_o%c...\"", *s);
    case PSEUDO_FIRST_OF:			/* After ':first_of' */
      if (*s == '-') {s++; state = PSEUDO_FIRST_OF_;}
      else errexit("Unknown pseudo-class \":first_of%c...\"", *s);
    case PSEUDO_FIRST_OF_:			/* After ':first_of_' */
      if (*s == 't' || *s == 'T') {s++; state = PSEUDO_FIRST_OF_T;}
      else errexit("Unknown pseudo-class \":first_of_%c...\"", *s);
    case PSEUDO_FIRST_OF_T:			/* After ':first_of_t' */
      if (*s == 'y' || *s == 'Y') {s++; state = PSEUDO_FIRST_OF_TY;}
      else errexit("Unknown pseudo-class \":first_of_t%c...\"", *s);
    case PSEUDO_FIRST_OF_TY:			/* After ':first_of_ty' */
      if (*s == 'p' || *s == 'P') {s++; state = PSEUDO_FIRST_OF_TYP;}
      else errexit("Unknown pseudo-class \":first_of_ty%c...\"", *s);
    case PSEUDO_FIRST_OF_TYP:			/* After ':first_of_typ' */
      if (*s == 'e' || *s == 'E') {s++; state = PSEUDO_FIRST_OF_TYPE;}
      else errexit("Unknown pseudo-class \":first_of_typ%c...\"", *s);
    case PSEUDO_FIRST_OF_TYPE:			/* After ':first_of_type' */
      if (!isnmchar(*s)) {sel->pseudos->type = FirstOfType; state =AFTER_TYPE;}
      else errexit("Unknown pseudo-class \":first_of_type%c...\"", *s);
    case PSEUDO_L:				/* After ':l' */
      if (*s == 'a' || *s == 'A') {s++; state = PSEUDO_LA;}
      else errexit("Unknown pseudo-class \":l%c...\"", *s);
      break;      
    case PSEUDO_LA:				/* After ':la' */
      if (*s == 'n' || *s == 'N') {s++; state = PSEUDO_LAN;}
      else errexit("Unknown pseudo-class \":la%c...\"", *s);
      break;      
    case PSEUDO_LAN:				/* After ':lan' */
      if (*s == 'g' || *s == 'G') {s++; state = PSEUDO_LANG;}
      else errexit("Unknown pseudo-class \":lan%c...\"", *s);
      break;      
    case PSEUDO_LANG:				/* After ':lang' */
      if (*s == '(') {s++; state = PSEUDO_LANG_;}
      else errexit("Unknown pseudo-class \":lang%c...\"", *s);
      break;      
    case PSEUDO_LANG_:				/* After ':lang(' */
      if (isspace(*s)) s++;
      else if (*s == '/') {s++; saved_state = PSEUDO_LANG_; state = SLASH;}
      else if (!isnmstart(*s)) errexit("Incorrect \":lang(\" at \"%c\"\n", *s);
      else {
	sel->pseudos->type = Lang;
	sel->pseudos->s = newstring("");
	strappc(&sel->pseudos->s, *s);
	s++;
	state = LANG;
      }
      break;
    case LANG:					/* Inside a language code */
      if (isnmchar(*s)) {strappc(&sel->pseudos->s, *s); s++;}
      else state = END_PSEUDO;
      break;
    default:
      assert(!"Cannot happen");
    }
  }
  return sel;
}
