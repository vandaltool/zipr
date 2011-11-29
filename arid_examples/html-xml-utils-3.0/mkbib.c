/*
 * mkbib - extract database entries from a db and format them
 *
 * mkbib reads a refer-style database of bibliographic entries, a list
 * of keys and a pattern file and outputs a list of citations
 * formatted according to the pattern and optionally sorted.
 *
 * The keys must correspond to %L fields in the refer database.
 *
 * The pattern file has the following structure:
 *
 * pattern: PREAMBLE entry POSTAMBLE;
 * entry: "{L:" [ TEXT | FIELD | conditional ]* "}";
 * conditional: "{" !"? F ":" [ TEXT | FIELD | conditional ]* "}";
 *
 * In the output, the entry will be repeated as often as there are
 * unique keys. A FIELD is of the form "%x" and wil be replaced by
 * field x of the entry.
 *
 * A part of the form "{x:ZZZ}" will be replaced by ZZZ if field x
 * exists and by nothing otherwise. A part of the form "{!x:ZZZ}" will
 * be replaced by ZZZ if field x does not exist.
 *
 * Occurrences of %x in the preamble (where x is a field name) will
 * not be output, but serve to build up the sort order. The default
 * sort order is to keep entries in the order they occur in the
 * auxfile, but if, e.g., "%A%D%T" occurs in the preamble, entries
 * will be sorted on author, date and title.
 *
 * To insert a literal "{", "}" or "%" in the preamble or in an entry,
 * prefix them with "%": "%{", "%}" and "%%".
 *
 * Usage: mkbib [-a auxfile] bibfile [inputfile]
 *
 * bibfile is a refer-style database.
 *
 * inputfile is the file that serves as template. If absent, stdin
 * is read.
 *
 * -a auxfile gives the name of the list of keys. If absent, the name
 * will be the same as inputfile with the extension (if any)
 * changed to ".aux". If no inputfile is given the default auxfile
 * is "aux.aux". Duplicate keys will only be used once.
 *
 * Note: When the "{x:" and "}" are inside an HTML file, they may be
 * in places where data is not allowed. To make the input file
 * itself valid HTML, it may be necessary to put them inside comments:
 * <!--{x:--> and <!--}-->. If one of them is put inside a comment,
 * the other must be as well.
 *
 * Here is an example of an input file:
 *
 * <html>
 * <title>Bibliography</title>
 * <!-- sort order is Author, Date, Title %A%D%T-->
 * <dl>
 * <!--{L:--><dt id="%L">%L
 * <dd>{A:%A.} <em>{T:%T.}</em> {D:%D. }
 * <!--}--></dl>
 * </html>
 *
 * To do: if the template has a period after a field and the field ends
 * in a period as well, only output one of the two.
 *
 * To do: if there are more than three authors or editors, only output
 * the first and add a string like "et al." or "and others"
 * (configurable).
 *
 * To do: if the template adds something like "(eds)", allow it to be
 * changed to "(ed)" if there is only one editor.
 *
 * Copyright © 1994-2004 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 19 March 2000
 * Version: $Id: mkbib.c,v 1.25 2004/04/29 15:48:56 bbos Exp $
 **/
#include <config.h>
#ifdef HAVE_ERRNO_H
#  include <errno.h>
#endif
#include <stdio.h>
#if STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
#endif
#include <stdlib.h>
#include <assert.h>

#ifdef HAVE_ERRNO_H
#  include <errno.h>
#endif
#ifdef HAVE_SEARCH_H
#  include <search.h>
#else
#  include "hash.e"		/* Use our own implementation */
#endif

#include <ctype.h>
#include "heap.e"
#include "types.e"
#include "errexit.e"


#define LINESIZE 32768
#define INCR 25
/* Warning: arbitrary limit! */
#define HASHSIZE 4096				/* Size of hash table */

static string prog;				/* argv[0] */
static string sortorder = NULL;			/* Default is unsorted */
static string separator = "; ";			/* Separates authors */
static int et_al_limit = 3;			/* Max # of authors to print */
static string et_al = "et al.";			/* String if more authors */


/* escape -- print a string, escaping characters dangerous for XML/HTML */
static void escape(const string s, char *last)
{
  int i;

  for (i = 0; s[i]; i++)
    switch (s[i]) {
      case '<': printf("&lt;"); break;
      case '>': printf("&gt;"); break;
      case '&': printf("&amp;"); break;
      case '"': printf("&quot;"); break;
      default: putchar(s[i]);
    }
  if (i > 0) *last = s[i-1];
}


/* put_field -- copy field field of entry with label key */
static void put_field(const string key, unsigned char field, char *last)
{
  ENTRY *e, e1 = {key, NULL};
  string *lines;
  int i, j, nrfields;

  /* ToDo: escape dangerous characters */
  /* ToDo: print "et. al." if more than N authors */
  /* ToDo: for fields other than %A and %E use only the last occurrence */
  /* ToDo: interpret and pretty-print dates in a consistent manner */

  if (field == '%' || field == '{' || field == '}') { /* Literal */
    putchar(field);
    *last = '\0';
    return;
  }

  /* Find the entry for key */
  if (! (e = hsearch(e1, FIND))) {
    fprintf(stderr, "%s: entry for key %s not found\n", prog, key);
    return;
  }

  /* Count how many occurences of %field there are in the entry */
  lines = (string*)e->data;			/* Type cast */
  for (i = 0, nrfields = 0; lines[i]; i++)
    if (lines[i][1] == field) nrfields++;

  /* Check that there is indeed a field */
  if (nrfields == 0) {
    fprintf(stderr, "%s: entry %s has no field %%%c\n", prog, key, field);
    return;
  }

  /* Check that there are no duplicate fields, other than for A and E */
  if (nrfields != 1 && ! (field == 'A' || field == 'E')) {
    fprintf(stderr, "%s: entry %s has duplicate field %%%c\n",
	    prog, key, field);
    return;
  }

  /* Now print the field(s) */
  if (nrfields > et_al_limit) {			/* Print only the first */
    for (i = 0; lines[i][1] != field; i++);	/* Find the first */
    escape(lines[i] + 3, last);			/* Print with entities */
    printf("%s%s", separator, et_al);
    *last = et_al[strlen(et_al) - 1];
  } else {					/* Print all fields */
    for (i = 0, j = 0; lines[i]; i++) {
      if (lines[i][1] == field) {		/* Found it */
	if (j != 0) printf("%s", separator); /* Multiple fields */
	escape(lines[i] + 3, last);		/* Print with entities */
	j++;
      }
    }
  }
}


/* get_field -- check that entry for key has a field f, return ptr to field */
static string get_field(const string key, const unsigned char f)
{
  ENTRY *e, e1 = {key, NULL};
  string *lines;
  int i;

  /* Find the entry for key */
  e = hsearch(e1, FIND);
  assert(e != NULL);
  assert(e->data != NULL);

  /* Find a line that starts with %field */
  lines = (string*)e->data;			/* Type cast */
  for (i = 0; lines[i] && lines[i][1] != f; i++) ;

  assert(! lines[i] || (lines[i][0] == '%' && lines[i][2] == ' '));
  return lines[i];
}


/* compare_keys -- return the relative sort order for two keys: -1, 0, 1 */
static int compare_keys(const void *aptr, const void *bptr)
{
  ENTRY e, *ae, *be;
  int c, i;
  string af, bf, a = *(string*)aptr, b = *(string*)bptr;

  /* Get the entry for key a */
  e.key = a;
  ae = hsearch(e, FIND);
  assert(ae != NULL);

  /* Get the entry for key b */
  e.key = b;
  be = hsearch(e, FIND);
  assert(be != NULL);

  /* Loop over sortorder, stop as soon as entries a and b are unequal */
  for (i = 0, c = 0; c == 0 && sortorder[i]; i++) {
    af = get_field(a, sortorder[i]);
    bf = get_field(b, sortorder[i]);
    c = strcmp(af ? af : (string)"", bf ? bf : (string)"");
  }

  return c;
}


/* sort_keys -- sort the keys according to the sort order given */
static void sort_keys(string *keys, const int n)
{
  assert(sortorder != NULL);
  qsort(keys, n, sizeof(*keys), compare_keys);
}


/* conditional -- conditionally copy a %{...%} segment */
static int conditional(const string pattern, const string key, char *last)
{
  Boolean on;
  int level, i = 1;

  /* Pattern starts with '{' */
  assert(pattern[0] == '{' && pattern[1] != '\0');

  /* Check the condition */
  if (pattern[i] == '!') on = !get_field(key, pattern[++i]);
  else on = get_field(key, pattern[i]) != NULL;

  if (pattern[i+1] != ':') errexit("%s: missing ':' in pattern\n", prog);

  /* Skip or copy until matching '%}' */
  if (! on) {					/* Skip until matching '}' */
    for (i += 2, level = 1; level != 0; i++)
      if (pattern[i] == '%') {
	if (pattern[++i] == '{') level++;
	else if (pattern[i] == '}') level--;
      }
    i--;					/* i points to '}' */
  } else {					/* Recursively copy segment */
    for (i += 2; True; i++)
      if (pattern[i] == '%') {
	if (pattern[++i] == '{') i += conditional(pattern + i, key, last);
	else if (pattern[i] == '}') break;
	else if (pattern[i] == '%') {putchar('%'); *last = '\0';}
	else put_field(key, pattern[i], last);
      } else if (*last != '.' || pattern[i] != '.') {
	putchar(pattern[i]);
	*last = '\0';
      } else {
	*last = '\0';				/* Don't print this '.' */
      }
  }
  
  return i;					/* Points at '}' */
}


/* copy -- copy pattern, expanding fields. (May sort keys) */
static void copy(const string pattern, string *keys, const int n)
{
  int j, start, end, level, slen = 0;
  char last = '\0';				/* Last char of field */

  assert(sortorder == NULL);

  /* ToDo: Find a way to declare the separator in the source. Maybe {&:...} */

  /* Find first '%{'. Also look for sort order */
  for (start = 0; pattern[start]; start++) {
    if (pattern[start] == '%') {		/* Special character */
      if (pattern[++start] == '{') {		/* Start of template */
	break;
      } else if ('A' <= pattern[start] && pattern[start] <= 'Z') {
	renewarray(sortorder, slen + 2);	/* Sort order */
	sortorder[slen] = pattern[start];
	sortorder[++slen] = '\0';
      } else {
	putchar('%');				/* Not special */
	putchar(pattern[start]);
      }
    } else {					/* Normal character */
      putchar(pattern[start]);
    }
  }
  if (!pattern[start]) {
    fprintf(stderr, "%s: warning: no '%%{' in input file\n", prog);
    return;					/* Nothing more to copy */
  }

  /* Sort the keys if there was a sort order */
  if (sortorder) sort_keys(keys, n);

  /* Start now points to '{'. Find matching '%}' */
  for (end = start + 1, level = 1; pattern[end] && level != 0; end++) {
    if (pattern[end] == '%') {
      if (pattern[++end] == '}') level--;
      else if (pattern[end] == '{') level++;
    }
  }
  if (level != 0) errexit("%s: unbalanced %{..%} in pattern\n", prog);

  /* End now points just after '}'. Loop over keys */
  for (j = 0; j < n; j++)
    conditional(pattern + start, keys[j], &last);
    
  /* Copy postamble */
  printf("%s", pattern + end);
}


/* in_list -- check if s is in the list of strings */
static Boolean in_list(const string s, const string *list, const int n)
{
  int i;

  for (i = 0; i < n && strcmp(s, list[i]) != 0; i++) ;
  return i < n;
}


/* read_keys -- read the list of keys from file f */
static string *read_keys(FILE *f, int *number)
{
  int i, e, n = 0;
  unsigned char line[LINESIZE];
  string *keys = NULL;

  clearerr(f);
  while (fgets(line, sizeof(line), f)) {

    /* Remove trailing \n and other whitespace */
    for (i = strlen(line); i > 0 && isspace(line[i-1]); i--) ;
    line[i] = '\0';

    /* ToDo: linear search fast enough? Books don't have 1000's of refs... */
    if (! in_list(line, keys, n)) {
      renewarray(keys, INCR * ((n + 1)/INCR + 1));
      keys[n++] = newstring(line);
    }
  }
  if ((e = ferror(f))) errexit("%s: %s\n", prog, strerror(e));

  *number = n;
  return keys;
}


/* check_and_store_entry -- check if we need this entry and if so store it */
static void check_and_store_entry(const string key, string *lines, int n)
{
  ENTRY e, *e1;

  renewarray(lines, INCR * ((n + 1)/INCR + 1));
  lines[n] = NULL;				/* Mark end of entry */
  if (key) {					/* Does it have a key at all */
    e.key = key;
    if ((e1 = hsearch(e, FIND))) 		/* Do we need this entry? */
      e1->data = (char*)lines;			/* Replace its data field */
  }
}


/* read_entries -- read the relevant entries from the refer database */
static void read_entries(FILE *f, const string *keys, const int n)
{
  unsigned char line[LINESIZE];
  string *lines = NULL;
  string key = NULL;
  ENTRY e, *e1;
  int i, j, fe;

  /* First enter all keys into the hash table without any data */
  for (i = 0; i < n; i++) {
    e.key = newstring(keys[i]);
    e.data = NULL;
    if (! hsearch(e, ENTER)) errexit("%s: %s\n", prog, strerror(errno));
  }

  /* Now read entries from the database */
  clearerr(f);
  i = 0;
  while (fgets(line, sizeof(line), f)) {

    if (line[0] != '%') {			/* Separator line */
      if (i != 0) {				/* We were in an entry */
	check_and_store_entry(key, lines, i);
	i = 0;					/* Reset */
	key = NULL;				/* Reset */
	lines = NULL;				/* Reset */
      }
    } else {					/* This line is a field */
      for (j = strlen(line); j > 0 && isspace(line[j-1]); j--) ;
      line[j] = '\0';				/* Remove trailing spaces */
      renewarray(lines, INCR * ((i + 1)/INCR + 1));
      lines[i] = newstring(line);
      if (strncmp(lines[i], "%L ", 3) == 0) key = lines[i] + 3;
      i++;
    }
  }
  if ((fe = ferror(f))) errexit("%s: %s\n", prog, strerror(fe));

  /* Check if last entry was already stored */
  if (i != 0)					/* We were still in an entry */
    check_and_store_entry(key, lines, i);

  /* Check that we found all keys */
  for (i = 0; i < n; i++) {
    e.key = keys[i];
    e1 = hsearch(e, FIND);
    assert(e1);
    if (! e1->data) errexit("%s: entry for \"%s\" not found\n", prog, keys[i]);
  }

}


/* read_pattern -- read the input file into memory */
static string read_pattern(FILE *f)
{
  string p = NULL;
  int n, len = 0;

  /* ToDo: use ferror to check for errors */
  do {
    renewarray(p, len + LINESIZE + 1);
    n = fread(p + len, sizeof(*p), LINESIZE, f);
    len += n;
  } while (! feof(f));
  p[len] = '\0';
  return p;
}


/* usage -- print usage message and exit */
static void usage(void)
{
  errexit("Version %s\nUsage: %s [-a auxfile] [-s sep] [-n maxauthors] [-r moreauthors] bibfile [inputfile]\n",
	  VERSION, prog);
}


/* main - main body */
int main(int argc, char *argv[])
{
  string auxfile = NULL, pattern, inputfile = NULL, dbfile, h;
  string *keys = NULL;
  FILE *f, *db, *aux;
  int i, n;

  /* Parse command line */
  prog = argv[0];
  for (i = 1; i < argc && argv[i][0] == '-' && !eq(argv[i], "--"); i++) {
    switch (argv[i][1]) {
    case 'a': auxfile = argv[++i]; break;
    case 's': separator = argv[++i]; break;
    case 'n': et_al_limit = atoi(argv[++i]); break;
    case 'r': et_al = argv[++i]; break;
    default: usage();
    }
  }
  if (i < argc && eq(argv[i], "--")) i++;
  if (i == argc || argc > i + 2) usage();

  /* First argument is refer database */
  dbfile = argv[i++];

  /* Optional second argument is input file */
  if (i != argc) inputfile = argv[i];

  /* If we don't have an explicit auxfile yet, derive its name */
  if (! auxfile) {
    if (! inputfile) {
      auxfile = "aux.aux";
    } else {
      newarray(auxfile, strlen(argv[i]) + 5);
      strcpy(auxfile, argv[i]);
      if ((h = strrchr(auxfile, '.'))) *h = '\0';
      strcat(auxfile, ".aux");
    }
  }

  /* Create a hash table */
  if (! hcreate(HASHSIZE)) errexit("%s: not enough memory for hash table\n", prog);
  
  /* Read keys from aux file */
  if (! (aux = fopen(auxfile, "r")))
    errexit("%s: %s: %s\n", prog, auxfile, strerror(errno));
  keys = read_keys(aux, &n);
  if (fclose(aux) != 0)
    errexit("%s: %s: %s\n", prog, auxfile, strerror(errno));

  /* Read the entries we need from the database */
  if (! (db = fopen(dbfile, "r")))
    errexit("%s: %s: %s\n", prog, dbfile, strerror(errno));
  read_entries(db, keys, n);
  if (fclose(db) != 0)
    errexit("%s: %s: %s\n", prog, dbfile, strerror(errno));

  /* Read pattern into memory */
  if (! (f = inputfile ? fopen(inputfile, "r") : stdin))
    errexit("%s: %s: %s\n", prog, inputfile, strerror(errno));
  pattern = read_pattern(f);
  if (fclose(f) != 0)
    errexit("%s: %s: %s\n", prog, inputfile, strerror(errno));

  /* Copy and expand the pattern */
  copy(pattern, keys, n);

  return 0;
}
