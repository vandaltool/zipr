/*
 * stackref_hash.h - see below.
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

#ifndef stackref_hash_h
#define stackref_hash_h

#include "all.h"


/*
 * necessary code for creating a hashtable of stack reference data
 */

extern Hashtable *stackrefs_hash;
typedef struct stackref_hash_key stackref_hash_key_t;
struct stackref_hash_key
{
        app_iaddr_t pc;
};

typedef struct stackref_hash_value stackref_hash_value_t;
struct stackref_hash_value
{
	int esp_offset;
	int size;
	stackref_hash_value_t *parent_stackref;
	stackref_hash_value_t *child_stackref;
	stackref_hash_value_t *sibling_stackref;
};

long stackrefs_compute_hash(void* key1);

long stackrefs_key_compare(void* key1, void* key2);

stackref_hash_value_t *add_stack_ref(app_iaddr_t pc,int size, int offset);
stackref_hash_value_t *add_stack_ref_field(stackref_hash_value_t *parent, app_iaddr_t pc,int size, int offset);

#endif

