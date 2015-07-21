/*
 * framerestore_hash.c - how to unwind frames.
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
#include "framerestore_hash.h"

Hashtable *framerestores_hash=NULL;

long framerestores_compute_hash(void* key1)
{
        framerestore_hash_key_t * a_key=(framerestore_hash_key_t *)key1;

        return a_key->pc;
}

long framerestores_key_compare(void* key1, void* key2)
{
        framerestore_hash_key_t * a_key=(framerestore_hash_key_t *)key1;
        framerestore_hash_key_t * b_key=(framerestore_hash_key_t *)key2;

        return a_key->pc == b_key->pc;
}


/*
 * frame_restore_set_return_address - set the offset of the return address for this frame
 */
void frame_restore_set_return_address(app_iaddr_t pc, int offset)
{
	instrmap_hash_value_t *imhv=(instrmap_hash_value_t*)Hashtable_get(instrmaps_hash,&pc);

	if(!imhv)
		return;

	framerestore_hash_value_t *frhv=(framerestore_hash_value_t*)Hashtable_get(framerestores_hash,&imhv->func_addr);

	if(!frhv)
	{
		framerestore_hash_key_t * frhk=(framerestore_hash_key_t*)spri_allocate_type(sizeof(framerestore_hash_key_t));
		frhv=(framerestore_hash_value_t*)spri_allocate_type(sizeof(framerestore_hash_value_t));
		assert(frhv);
		memset(frhv,0, sizeof(*frhv));


		frhk->pc=imhv->func_addr;

		Hashtable_put(framerestores_hash, frhk, frhv);
	}

	frhv->ret_offset=offset;
}



/* 
 * frame_restore_hash_add_reg_restore - add info to the frame restore hash about the type and offset of saved registers
 */
void frame_restore_hash_add_reg_restore(app_iaddr_t addr, int reg_num, int reg_offset, int reg_type)
{

        framerestore_hash_value_t *frhv=(framerestore_hash_value_t*) Hashtable_get(framerestores_hash,&addr);

        if(!frhv)
        {
                framerestore_hash_key_t * frhk=(framerestore_hash_key_t*)spri_allocate_type(sizeof(framerestore_hash_key_t));
                frhv=(framerestore_hash_value_t*)spri_allocate_type(sizeof(framerestore_hash_value_t));

		assert(frhv);
		memset(frhv,0, sizeof(*frhv));


                frhk->pc=addr;

                Hashtable_put(framerestores_hash, frhk, frhv);
        }

        frhv->reg_offsets[reg_num]=reg_offset;
        frhv->reg_types[reg_num]=reg_type;

	assert(reg_num>=0 && reg_num<=15);
	assert(reg_type>=0 && reg_type<=255);
	assert(reg_offset<=0);
	
}


void frame_restore_hash_set_safe_bit(app_iaddr_t addr, int is_safe)
{

        framerestore_hash_value_t *frhv=(framerestore_hash_value_t*)Hashtable_get(framerestores_hash,&addr);

        if(!frhv)
        {
                framerestore_hash_key_t * frhk=(framerestore_hash_key_t*)spri_allocate_type(sizeof(framerestore_hash_key_t));
                frhv=(framerestore_hash_value_t*)spri_allocate_type(sizeof(framerestore_hash_value_t));

		assert(frhv);
		memset(frhv,0, sizeof(*frhv));


                frhk->pc=addr;

                Hashtable_put(framerestores_hash, frhk, frhv);
        }

        frhv->static_analyzer_believes_safe=is_safe;
}

int is_safe_function(app_iaddr_t pc)
{
	instrmap_hash_value_t *imhv=(instrmap_hash_value_t*)Hashtable_get(instrmaps_hash,&pc);

	if(!imhv)
		return FALSE;

	framerestore_hash_value_t *frhv=(framerestore_hash_value_t*)Hashtable_get(framerestores_hash,&imhv->func_addr);

	if(!frhv)
		return FALSE;

	return frhv->static_analyzer_believes_safe;
}
