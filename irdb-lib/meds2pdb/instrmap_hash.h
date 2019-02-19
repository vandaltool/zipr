/*
 * instrmap_hash.h - hash table for instrumentation details.
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

#ifndef instrmap_hash_h
#define instrmap_hash_h

#include "meds_all.h"


extern Hashtable *instrmaps_hash;
struct instrmap_hash_key
{
        int pc;
};
typedef struct instrmap_hash_key instrmap_hash_key_t;

struct instrmap_hash_value
{
	int func_addr;
	int site_alloc;
        int size;
};
typedef struct instrmap_hash_value instrmap_hash_value_t;

long instrmaps_compute_hash(void* key1);

long instrmaps_key_compare(void* key1, void* key2);

#endif
