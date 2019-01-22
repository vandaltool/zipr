/*
 * constant_hash.c - hashtable for contants.
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

#include "meds_all.h"

Hashtable *constants_hash=NULL;

long constants_compute_hash(void* key1)
{
        constant_hash_key_t * a_key=(constant_hash_key_t *)key1;

        return a_key->pc ^ a_key->the_const ^ (a_key->field << 4);
}

long constants_key_compare(void* key1, void* key2)
{
        constant_hash_key_t * a_key=(constant_hash_key_t *)key1;
        constant_hash_key_t * b_key=(constant_hash_key_t *)key2;

        if(a_key->pc == b_key->pc)
	{
		if(a_key->field==b_key->field)
                	return a_key->the_const == b_key->the_const;
		else
        		return a_key->field == b_key->field;
	}
        return a_key->pc == b_key->pc;
}


constant_hash_value_t * add_constant_ref(libIRDB::virtual_offset_t pc,int the_const, constant_hash_field_t the_field, constant_hash_type_t the_type)
{
        constant_hash_key_t *chk=(constant_hash_key_t*)spri_allocate_type(sizeof(constant_hash_key_t ));
        constant_hash_value_t *chv=(constant_hash_value_t*)spri_allocate_type(sizeof(constant_hash_value_t ));
        /* set the key */
        chk->pc=pc;
        chk->the_const=the_const;
        chk->field=the_field;
        /* set the data */
        chv->type=the_type;

        Hashtable_put(constants_hash, chk, chv);
	return chv;

}

constant_hash_value_t * get_constant_ref(libIRDB::virtual_offset_t pc, int the_const, constant_hash_field_t the_field)
{
	constant_hash_key_t chk={pc, the_const, the_field};
	constant_hash_value_t *chv=(constant_hash_value_t*)Hashtable_get(constants_hash, &chk);

        return  chv ? chv : 0;

}


const char* constant_hash_type_to_string(constant_hash_type_t type)
{
	switch(type)
	{
		case cht_ESP: 	return "PTRIMMEDESP2 STACK";
		case cht_EBP: 	return "PTRIMMEDEBP2 STACK";
		case cht_GLOBAL:	return "PTRIMMEDABSOLUTE GLOBAL";
		case cht_NUMBER: 	return "NUMIMMED GLOBAL";
		case cht_UNKNOWN:	return "UNKNOWNIMMED GLOBAL";
		default:
			assert(0);
	}
}

