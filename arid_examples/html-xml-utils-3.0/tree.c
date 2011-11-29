/*
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 1997
 * Version: $Id: tree.c,v 1.34 2003/12/04 13:23:14 bbos Exp $
 **/
#include <config.h>
#include <assert.h>
#include <stdlib.h>
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
#include <ctype.h>
#include <stdio.h>
#include "export.h"
#include "heap.e"
#include "types.e"
#include "dtd.e"

EXPORT typedef enum {
  Element, Text, Comment, Declaration, Procins, Root
} Nodetype;

EXPORT typedef struct _node {
  Nodetype tp;
  string name;
  pairlist attribs;
  string text;
  string url;
  struct _node *parent;
  struct _node *sister;
  struct _node *children;
} Node, *Tree;

  

/* down -- convert a string to lowercase, return pointer to arg */
static inline string down(string s)
{
  string t;
  for (t = s; *t; t++) *t = tolower(*t);
  return s;
}

/* create -- create an empty tree */
EXPORT Tree create(void)
{
  Tree t = malloc(sizeof(*t));
  assert(t != NULL);
  t->tp = Root;
  t->name = "";
  t->children = t->children = NULL;
  return t;
}

/* tree_delete -- recursively free the memory occupied by a tree */
EXPORT void tree_delete(Tree t)
{
  if (t != NULL) {
    switch (t->tp) {
      case Element:
	dispose(t->name);
	pairlist_delete(t->attribs);
	tree_delete(t->sister);
	tree_delete(t->children);
	break;
      case Text:
	dispose(t->text);
	assert(t->children == NULL);
	tree_delete(t->sister);
	break;
      case Comment:
	dispose(t->text);
	assert(t->children == NULL);
	tree_delete(t->sister);
	break;
      case Declaration:
	dispose(t->name);
	dispose(t->text);
	dispose(t->url);
	assert(t->children == NULL);
	tree_delete(t->sister);
	break;
      case Procins:
	dispose(t->text);
	assert(t->children == NULL);
	tree_delete(t->sister);
	break;
      case Root:
	assert(t->sister == NULL);
	tree_delete(t->children);
	break;
      default:
	assert(!"Cannot happen");
    }
    dispose(t);
  }
}

/* get_root -- return root of tree */
EXPORT Tree get_root(Tree t)
{
  while (t->tp != Root) t = t->parent;
  return t;
}

/* get_attrib -- return a ptr to the value of a named attibute, or False */
EXPORT Boolean get_attrib(Node *e, const string attname, string *val)
{
  pairlist h;

  assert(e->tp == Element);
  for (h = e->attribs; h != NULL; h = h->next) {
    if (strcasecmp(h->name, attname) == 0) {
      if (val != NULL) *val = h->value;
      return True;
    }
  }
  return False;
}

/* set_attrib -- set an attribute to a value */
EXPORT void set_attrib(Node *e, string name, string value)
{
  pairlist h, p;
  assert(e->tp == Element);
  for (h = e->attribs; h != NULL; h = h->next) {
    if (strcasecmp(h->name, name) == 0) {
      free(h->value);
      h->value = strdup(value);
      return;
    }
  }
  h = malloc(sizeof(*h));
  assert(h != NULL);
  h->name = strdup(name);
  down(h->name);
  h->value = strdup(value);
  h->next = NULL;
  /* Insert into sorted list */
  /* ToDo: the list is not sorted by html_push; do we need sorting? */
  if (e->attribs == NULL || strcmp(h->name, e->attribs->name) < 0) {
    h->next = e->attribs;
    e->attribs = h;
  } else {
    p = e->attribs;
    while (p->next != NULL && strcmp(h->name, p->next->name) > 0) p = p->next;
    h->next = p->next;
    p->next = h;
  }
}

/* wrap_contents -- wrap contents of a node in an element, return new elt */
EXPORT Tree wrap_contents(Node *n, const string elem, pairlist attr)
{
  Node *h, *k;

  new(h);
  h->tp = Element;
  h->name = down(newstring(elem));
  h->attribs = attr;
  h->sister = NULL;
  h->parent = n;
  h->children = n->children;
  n->children = h;
  for (k = h->children; k; k = k->sister) k->parent = h;
  return h;
}

/* push -- add a child node to the tree */
static Tree push(Tree t, Node *n)
{
  if (t->children == NULL) {
    t->children = n;
  } else {
    Tree h = t->children;
    while (h->sister != NULL) h = h->sister;
    h->sister = n;
  }
  n->parent = t;
  return n;
}

/* pop -- go up one level */
static Tree pop(Tree t)
{
  assert(t != NULL);
  assert(t->tp != Root);
  return t->parent;
}

/* append -- add at end of children */
static Tree append(Tree t, Node *n)
{
  assert(t != NULL);
  if (t->children == NULL) {
    t->children = n;
  } else {
    Tree h = t->children;
    while (h->sister != NULL) h = h->sister;
    h->sister = n;
  }
  n->parent = t;
  return t;
}

/* lookup -- lookup info about an element case-insensitively */
static const ElementType *lookup(const string e)
{
  unsigned char h[MAXNAMELEN+2];
  down(strncpy(h, e, sizeof(h) - 1));
  return lookup_element(h, strlen(e));
}

/* is_known -- true if the element is an HTML 4 element */
EXPORT Boolean is_known(const string e)
{
  return lookup(e) != NULL;
}

/* is_pre -- true if the element has preformatted content */
EXPORT Boolean is_pre(const string e)
{
  const ElementType *info = lookup(e);
  return info && info->pre;
}

/* need_stag -- true if the element's start tag is required */
EXPORT Boolean need_stag(const string e)
{
  const ElementType *info = lookup(e);
  return !info || info->stag;
}

/* need_etag -- true if the element's end tag is required */
EXPORT Boolean need_etag(const string e)
{
  const ElementType *info = lookup(e);
  return !info || info->etag;
}

/* is_empty -- true if element is empty */
EXPORT Boolean is_empty(const string e)
{
  const ElementType *info = lookup(e);
  return info && info->empty;
}

/* has_parent -- true if c accepts p as a parent */
static Boolean has_parent(const char *c, const char *p)
{
  const ElementType *info = lookup_element(c, strlen(c));
  int i;
  if (!info) return False;
  for (i = 0; info->parents[i]; i++)
    if (eq(info->parents[i], p)) return True;
  return False;
}

/* preferred_parent -- return first possible parent of e */
static char *preferred_parent(const char *e)
{
  const ElementType *info = lookup_element(e, strlen(e));
  assert(info != NULL);				/* element is known */
  assert(info->parents[0] != NULL);		/* element is not root */
  return info->parents[0];
}
  
/* is_root -- true if e has no possible parents */
static Boolean is_root(const char *e)
{
  const ElementType *info = lookup_element(e, strlen(e));
  assert(info != NULL);				/* element is known */
  return info->parents[0] == NULL;
}
  
/* is_mixed -- true if e accepts text content */
EXPORT Boolean is_mixed(const string e)
{
  const ElementType *info = lookup(e);
  return !info || info->mixed;
}
  
/* break_before -- true if element looks better with a newline before it */
EXPORT Boolean break_before(const string e)
{
  const ElementType *info = lookup(e);
  return info && info->break_before;
}
  
/* break_after -- true if element looks better with a newline after it */
EXPORT Boolean break_after(const string e)
{
  const ElementType *info = lookup(e);
  return info && info->break_after;
}

/* build_path -- try to add omittable start tags to make elem acceptable */
static Boolean build_path(Tree *t, string elem)
{
  const ElementType *info;
  Node *n;
  int i;

  assert(is_known(elem));
  assert(is_known((*t)->name));

  /* Check if we are done */
  if (has_parent(elem, (*t)->name)) return True;
  
  /* Try recursively if any possible parent can be a child of t */
  info = lookup(elem);
  for (i = 0; info->parents[i]; i++) {
    if (!need_stag(info->parents[i]) && build_path(t, info->parents[i])) {
      /* Success, so add this parent and return True */
      n = malloc(sizeof(*n));
      assert(n != NULL);
      n->tp = Element;
      n->name = newstring(info->parents[i]);
      assert(islower(n->name[0]));
      n->attribs = NULL;
      n->sister = n->children = NULL;
      *t = push(*t, n);
      return True;
    }
  }
  return False;
}

/* html_push -- add an element to the tree, open or close missing elements */
EXPORT Tree html_push(Tree t, string elem, pairlist attr)
{
  pairlist a;
  Node *h, *n = malloc(sizeof(*n));
  assert(n != NULL);
  n->tp = Element;
  n->name = down(newstring(elem));
  for (a = attr; a; a = a->next) down(a->name);
  n->attribs = attr;
  n->sister = n->children = NULL;

  /* Unknown elements are just pushed where they are */
  if (!is_known(n->name)) return push(t, n);

  if (is_root(n->name)) {
    while (t->tp != Root) t = pop(t);		/* Make sure root is at root */
  } else if (is_known(t->name) && build_path(&t, n->name)) {
    ;						/* Added missing start tags */
  } else {
    /* Check if there is a possible parent further up the tree */
    for (h=t; h->tp!=Root && is_known(h->name) && !has_parent(n->name,h->name);
	 h = h->parent) ;
    /* Close omitted end tags */
    if (h->tp != Root) while (t != h) t = pop(t);
    /* If no valid parent, fabricate one */
    if (t->tp == Root || (is_known(t->name) && !has_parent(n->name, t->name)))
      t = html_push(t, preferred_parent(n->name), NULL);
  }
  t = push(t, n);

  if (is_empty(n->name)) t = pop(t);
  return t;
}

/* html_pop -- close an open element */
EXPORT Tree html_pop(Tree t, string elem)
{
  Tree h = t;
  assert(t != NULL);
  down(elem);
  if (*elem == '\0') {				/* </> */
    if (t->tp != Root) t = pop(t);
  } else {					/* </name> */
    for (h = t; h->tp != Root && !eq(h->name, elem); h = h->parent) ;
    if (h->tp != Root) {			/* Found open element */
      while (t != h) t = pop(t);
      t = pop(t);
    }
  }
  return t;
}

/* append_comment -- add a comment to the tree */
EXPORT Tree append_comment(Tree t, string comment)
{
  Node *n = malloc(sizeof(*n));
  assert(n != NULL);
  n->tp = Comment;
  n->text = comment;
  n->sister = n->children = NULL;
  return append(t, n);
}

/* append_declaration -- add a declaration to the tree */
EXPORT Tree append_declaration(Tree t, string gi,
			       string fpi, string url)
{
  Node *n = malloc(sizeof(*n));
  assert(n != NULL);
  n->tp = Declaration;
  n->name = down(gi);
  n->text = fpi;
  n->url = url;
  n->sister = n->children = NULL;
  return append(t, n);
}

/* append_procins -- append a processing instruction */
EXPORT Tree append_procins(Tree t, string procins)
{
  Node *n = malloc(sizeof(*n));
  assert(n != NULL);
  n->tp = Procins;
  n->text = procins;
  n->sister = n->children = NULL;
  return append(t, n);
}

/* is_whitespace -- true if whole string consists of whitespace */
static Boolean is_whitespace(string s)
{
  for (;*s != '\0'; s++)
    if (*s != ' ' && *s != '\t' && *s != '\r' && *s != '\n') return False;
  return True;
}

/* append_text -- append a text chunk to the document tree */
EXPORT Tree append_text(Tree t, string text)
{
  Node *n;
  string new_parent;

  if (is_whitespace(text)
      && (t->tp == Root || !is_mixed(t->name))) {
    /* Drop text, since it is non-significant whitespace */
    return t;
  }
  if (t->tp == Root || !is_mixed(t->name)) {
    /* Need heuristics to make a valid tree */
    new_parent = preferred_parent("%data");
    /* Close omitted end tags until text or preferred parent fits */
    while (t->tp != Root && !is_mixed(t->name) && !need_etag(t->name)
	   && !has_parent(new_parent, t->name))
      t = pop(t);
    /* Fabricate a parent if needed */
    if (t->tp == Root || !is_mixed(t->name))
      t = html_push(t, new_parent, NULL);
  }
  n = malloc(sizeof(*n));
  assert(n != NULL);
  n->tp = Text;
  n->text = text;
  assert(n->text != NULL);
  n->sister = n->children = NULL;
  return append(t, n);
}

static void dump2(Tree n)
{
  pairlist h;
  Tree l;

  switch (n->tp) {
    case Text: printf("%s", n->text); break;
    case Comment: printf("<!--%s-->", n->text); break;
    case Declaration:
      printf("<!DOCTYPE %s", n->name);
      if (n->text) printf(" \"%s\"", n->text); else printf(" SYSTEM");
      if (n->url) printf(" \"%s\"", n->url);
      printf(">\n");
      break;
    case Procins: printf("<?%s>", n->text); break;
    case Element:
      printf("<%s", n->name);
      for (h = n->attribs; h != NULL; h = h->next) {
	printf(" %s", h->name);
	if (h->value != NULL) printf("=\"%s\"", h->value);
      }
      if (is_empty(n->name)) {
	assert(n->children == NULL);
	printf(" />");
      } else {
	printf(">");
	for (l = n->children; l != NULL; l = l->sister) dump2(l);
	printf("</%s>", n->name);
      }
      break;
    default:
      assert(!"Cannot happen");
  }
}

/* dumptree -- write out the tree below t (t's children, not t itself)*/
EXPORT void dumptree(Tree t)
{
  Tree h;

  for (h = t->children; h != NULL; h = h->sister) dump2(h);
}
