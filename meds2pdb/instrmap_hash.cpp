/*
 * instrmap_hash.c - hash table for instrumentation details.
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

#include "instrmap_hash.h"

Hashtable *instrmaps_hash=NULL;

long instrmaps_compute_hash(void* key1)
{
        instrmap_hash_key_t * a_key=(instrmap_hash_key_t *)key1;

        return a_key->pc;
}

long instrmaps_key_compare(void* key1, void* key2)
{
        instrmap_hash_key_t * a_key=(instrmap_hash_key_t *)key1;
        instrmap_hash_key_t * b_key=(instrmap_hash_key_t *)key2;

        return a_key->pc == b_key->pc;
}


