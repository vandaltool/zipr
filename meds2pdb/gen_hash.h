/*
 * gen_hash.h - generic hash table code.
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


// Hashtable.h

#ifndef HASHTABLE_H
#define HASHTABLE_H


// opaque structure
typedef struct gen_hashtable Hashtable;

struct entry
{
    void *key;
    void *value;
    struct entry *next;
};

struct gen_hashtable
{
    struct entry **table;
    long tableLength;
    long count;
    long threshold;
    long (*hash_func)(void*);
    long (*key_compare)(void*, void*);
};

struct gen_hashtable_iterator
{
    Hashtable*     ht;
    long           idx;
    struct entry*  current;
};

typedef struct gen_hashtable_iterator Hashtable_iterator;

/**
 * Creates an empty hashtable.
 */
Hashtable *Hashtable_create(long (*hash)(void*), long (*key_compare)(void*,void*));

/**
 * Free the hashtable memory.
 */
void Hashtable_destroy( Hashtable **h );

/**
 * Puts the key, value pair into the hashtable.
 */
void Hashtable_put( Hashtable *h, void *key, void *value );

/**
 * Returns the value associated with the key.
 */
void *Hashtable_get( const Hashtable *h, void *key );

/**
 * Returns the number of values in the hashtable.
 */
long Hashtable_size( const Hashtable *h );


/**
 * Returns TRUE if the hashtable contains the specified key.
 */
int Hashtable_containsKey( const Hashtable *h, void *key );

/**
 * Hash table iterator
 */

Hashtable_iterator Hashtable_setup_iterator( Hashtable *h );
struct entry* Hashtable_get_next(Hashtable_iterator &i);

#endif

