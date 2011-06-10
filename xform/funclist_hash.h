/*
 * funclist_hash.h - list of functions hashtable.
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

#ifndef funclist_hash_h
#define funclist_hash_h

#include "all.h"


extern Hashtable *funclists_hash;
struct funclist_hash_key
{
	char* name;
	
};
typedef struct funclist_hash_key funclist_hash_key_t;

struct funclist_hash_value
{
        int pc;
};
typedef struct funclist_hash_value funclist_hash_value_t;

long funclists_compute_hash(void* key1);

long funclists_key_compare(void* key1, void* key2);

#endif
