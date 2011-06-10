/*
 * gen_hash.c - generic hash table code.
 *
 * Copyright (c) 2000, 2001, 2010 - University of Virginia 
 *
 * This file is part of the Memory Error Detection System (MEDS) infrastructure.
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

/*
 *  gen_hash.c -- generic hashtable implmentation.
 */



#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "gen_hash.h"
#include "spri_alloc.h"

#define INITIAL_CAPACITY 128

#define HT_FREE(p,s) (spri_deallocate_type(p,s))
#define HT_MALLOC(s) (spri_allocate_type(s))
#define HT_CALLOC(n,s)  (memset((void*)spri_allocate_type((n)*(s)),0,((n)*(s))))

#define TRUE 1
#define FALSE 0


Hashtable *Hashtable_create( long (*hash_func)(void* ), long (*key_compare)(void*, void*))
{
    Hashtable *result = (Hashtable*)HT_MALLOC( sizeof(Hashtable) );
    result->table = (struct entry**)HT_CALLOC( INITIAL_CAPACITY, sizeof(struct entry*) );
    result->hash_func=hash_func;
    result->key_compare=key_compare;
    result->tableLength = INITIAL_CAPACITY;

    result->count = 0;
    result->threshold = (long)(INITIAL_CAPACITY * 3 / 4);	/* integer calcs for 3/4ths full */

    return result;
}

void Hashtable_destroy( Hashtable **h )
{
assert(0);
#if 0
    long i;
    for( i = (*h)->tableLength - 1; i >= 0; i-- )
    {
        struct entry *e = (*h)->table[i];
        while( e != NULL )
        {
            struct entry *deleteEntry = e;
            e = e->next;
            HT_FREE( deleteEntry, sizeof(struct entry*) );
        }
    }
    HT_FREE( (*h)->table, h->tableLength*sizeof(struct entry*) );
    HT_FREE( *h, sizeof(Hashtable) );
    *h = NULL;
#endif
}

void Hashtable_rehash( Hashtable *h )
{
    long i;
    struct entry **oldTable = h->table;
    long newCapacity = h->tableLength * 2;
    struct entry **newTable = (struct entry**)HT_CALLOC( newCapacity, sizeof(struct entry*) );

    h->threshold = (long)(newCapacity * 3 / 4); /* integer calcs for 3/4ths full */

    for( i = h->tableLength - 1; i >= 0; i-- )
    {
        struct entry *old;
        for( old = oldTable[i]; old != NULL; )
        {
            struct entry *e = old;
            long index = h->hash_func(e->key) & (newCapacity - 1) ;
            old = old->next;

            e->next = newTable[index];
            newTable[index] = e;
        }
    }

    HT_FREE( h->table, h->tableLength*sizeof(struct entry*) );
    h->table = newTable;
    h->tableLength = newCapacity;
}

void Hashtable_put( Hashtable *h, void *key, void *value )
{
    struct entry *e;
    long hash = (long)key;
    long index = h->hash_func(key) & (h->tableLength-1);

    // make sure the key isn't already present
    for( e = h->table[index]; e != NULL; e = e->next )
    {
        if( h->key_compare(e->key , key ))
        {
            e->value = value;
            return;
        }
    }

    if( h->count >= h->threshold )
    {
        Hashtable_rehash( h );
        Hashtable_put( h, key, value );
    }
    else
    {
        e = (struct entry *)HT_MALLOC( sizeof(struct entry) );
        e->key = key;
        e->value = value;
        e->next = h->table[index];
        h->table[index] = e;
        h->count++;
    }
}

void *Hashtable_get( const Hashtable *h, void *key )
{
    struct entry *e;
    long index = h->hash_func(key) & (h->tableLength-1);

    for( e = h->table[index]; e != NULL; e = e->next )
    {
        if( h->key_compare(e->key , key ))
            return e->value;
    }
    return NULL;
}

long Hashtable_size( const Hashtable *h )
{
    return h->count;
}



int Hashtable_containsKey( const Hashtable *h, void *key )
{
    return Hashtable_get( h, key ) == NULL ? FALSE : TRUE;
}

/*
 * Initialize iterator
 */
Hashtable_iterator Hashtable_setup_iterator( Hashtable *h )
{
  Hashtable_iterator i;
  i.ht = h;
  i.idx = -1;
  i.current = NULL;
  return i;
}

/*
*  Returns next entry in hash table
*/
struct entry* Hashtable_get_next(Hashtable_iterator &iterator)
{
    if (!iterator.ht)
    {
        fprintf(stderr,"Hashtable_get_next: cannot find the hash table\n");
        return NULL;
    }
    else if (iterator.ht->count <= 0)
    {
      fprintf(stderr,"Hashtable_get_next: the hashtable count is <= 0   0x%x\n", iterator.ht);
      return NULL;
    }

    if (iterator.idx < 0)
    {
      int i;
      // first time, find the first real entry
      for (i = 0; i < iterator.ht->tableLength; ++i)
      {
        if (iterator.ht->table[i] != NULL)
        {
          iterator.idx = i;
          iterator.current = iterator.ht->table[i];
          return iterator.current;
        }
      }
      fprintf(stderr,"Hashtable_get_next: no keys found in hash table\n");
      return NULL; // empty table, no keys found
    }
    else
    {
        if (iterator.current)
        { 
            if (iterator.current->next)
            {
                iterator.current = iterator.current->next;
                return iterator.current;
            }
            else
            {
                int i;
                // get the next entry
                for (i = iterator.idx + 1; i < iterator.ht->tableLength; ++i)
                {
                    if (iterator.ht->table[i] != NULL)
                    {
                        iterator.idx = i;
                        iterator.current = iterator.ht->table[i];
                        return iterator.current;
                    }
                }
                return NULL; // we're done
            }
        }
    } 
} 

