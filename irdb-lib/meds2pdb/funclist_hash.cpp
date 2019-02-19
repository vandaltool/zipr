/*
 * funclist_hash.c - list of functions hashtable.
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

#include <string.h>
#include "funclist_hash.h"

Hashtable *funclists_hash=NULL;

long funclists_compute_hash(void* key1)
{
        funclist_hash_key_t * a_key=(funclist_hash_key_t *)key1;
	char *s =a_key->name;

	int h = 0;
	while (*s)
	{
    		h = 31*h + *s;
		s++;
	}
	return h;
}

long funclists_key_compare(void* key1, void* key2)
{
        funclist_hash_key_t * a_key=(funclist_hash_key_t *)key1;
        funclist_hash_key_t * b_key=(funclist_hash_key_t *)key2;

        return !strcmp(a_key->name, b_key->name);
}


