#ifndef HAVE_SEARCH_H
/*
 * hsearch() on Mac OS X 10.1.2 appears to be broken: there is no
 * search.h; there is a search() in the C library, but it doesn't work
 * properly. We include some hash functions here, protected by
 * HAVE_SEARCH_H. Hopefully when search.h appears in Mac OS X,
 * hsearch() will be fixed at the same time...
 */

#include "config.h"
#include <stdlib.h>
#include <assert.h>
#include "export.h"
#include "heap.e"


EXPORT typedef struct entry {char *key; void *data;} ENTRY;
EXPORT typedef enum {FIND, ENTER} ACTION;

static ENTRY *htab;
static int *htab_index1, *htab_index2;
static unsigned int htab_size = 0;
static unsigned int htab_inited;


/* isprime -- test if n is a prime number */
static int isprime(unsigned int n)
{
  /* Simplistic algorithm, probably good enough for now */
  unsigned int i;
  assert(n % 2);				/* n not even */
  for (i = 3; i * i < n; i += 2) if (n % i == 0) return 0;
  return 1;
}


/* hcreate -- create a hash table for at least nel entries */
EXPORT int hcreate(size_t nel)
{
  /* Change nel to next higher prime */
  for (nel |= 1; !isprime(nel); nel += 2) ;

  /* Allocate hash table and array to keep track of initialized entries */
  newarray(htab, nel);
  newarray(htab_index1, nel);
  newarray(htab_index2, nel);
  if (!htab || !htab_index1 || !htab_index2) {
    dispose(htab);
    dispose(htab_index1);
    dispose(htab_index2);
    return 0;			/* Out of memory */
  }
  htab_inited = 0;
  htab_size = nel;
  return 1;
}


/* hdestroy -- deallocate hash table */
EXPORT void hdestroy(void)
{
  assert(htab_size);
  dispose(htab_index1);
  dispose(htab_index2);
  dispose(htab);
  htab_size = 0;
}


/* hsearch -- search for and/or insert an entry in the hash table */
EXPORT ENTRY *hsearch(ENTRY item, ACTION action)
{
  unsigned int hval, i;
  char *p;

  assert(htab_size);				/* There must be a hash table */

  /* Compute a hash value */
#if 1
  /* This function suggested by Dan Bernstein */
  for (hval = 5381, p = item.key; *p; p++) hval = (hval * 33) ^ *p;
#else
  i = hval = strlen(item.key);
  do {i--; hval = (hval << 1) + item.key[i];} while (i > 0);
#endif
  hval %= htab_size;
  /* if (action == ENTER) debug("%d\n", hval); */

  /* Look for either an empty slot or an entry with the wanted key */
  i = hval;
  while (htab_index1[i] < htab_inited
	 && htab_index2[htab_index1[i]] == i
	 && strcmp(htab[i].key, item.key) != 0) {
    i = (i + 1) % htab_size;			/* "Open" hash method */
    if (i == hval) return NULL;			/* Made full round */
  }
  /* Now we either have an empty slot or an entry with the same key */
  if (action == ENTER) {
    htab[i].key = item.key;			/* Put the item in this slot */
    htab[i].data = item.data;
    if (htab_index1[i] >= htab_inited || htab_index2[htab_index1[i]] != i) {
      /* Item was not yet used, mark it as used */
      htab_index1[i] = htab_inited;
      htab_index2[htab_inited] = i;
      htab_inited++;
    }
    return &htab[i];
  } else if (htab_index1[i] < htab_inited && htab_index2[htab_index1[i]] == i)
    return &htab[i];				/* action == FIND, found key */

  return NULL;					/* Found empty slot */
}

#endif /* HAVE_SEARCH_H */
