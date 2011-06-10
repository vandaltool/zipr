/*
 * framerestore_hash.h - how to unwind frames.
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

#ifndef framerestore_hash_h
#define framerestore_hash_h

#include "all.h"


extern Hashtable *framerestores_hash;
struct framerestore_hash_key
{
        int pc;	// function address
};
typedef struct framerestore_hash_key framerestore_hash_key_t;

struct framerestore_hash_value
{
	int reg_offsets[8];	/* index by MD_REG_*, 0=do not restore, else add offset to ret_addr offset */
	int reg_types[8];	/* index by MD_REG_*, return type of each register  */

	int ret_offset;

	int static_analyzer_believes_safe;
};

typedef struct framerestore_hash_value framerestore_hash_value_t;

long framerestores_compute_hash(void* key1);

long framerestores_key_compare(void* key1, void* key2);

void frame_restore_hash_add_reg_restore(app_iaddr_t addr, int reg_num, int reg_offset, int reg_type);

void frame_restore_hash_set_safe_bit(app_iaddr_t addr, int is_safe);

void frame_restore_hash_set_frame_size(app_iaddr_t addr, int is_safe);
void frame_restore_set_return_address(app_iaddr_t pc, int offset);


#endif
