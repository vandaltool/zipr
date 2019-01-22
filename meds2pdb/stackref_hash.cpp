/*
 * stackref_hash.c - see below.
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
#include "stackref_hash.h"

/* 
 * hashtable of data for stack referents 
 */

Hashtable *stackrefs_hash=NULL;

long stackrefs_compute_hash(void* key1)
{
        stackref_hash_key_t * a_key=(stackref_hash_key_t *)key1;

        return a_key->pc;
}

long stackrefs_key_compare(void* key1, void* key2)
{
        stackref_hash_key_t * a_key=(stackref_hash_key_t *)key1;
        stackref_hash_key_t * b_key=(stackref_hash_key_t *)key2;

        return a_key->pc == b_key->pc;
}


/* add something to the stackrefs_hash hashtable */
stackref_hash_value_t *add_stack_ref(libIRDB::virtual_offset_t pc,int size, int offset)
{
       	stackref_hash_key_t *sshk=(stackref_hash_key_t*)spri_allocate_type(sizeof(stackref_hash_key_t ));
       	stackref_hash_value_t *sshv=(stackref_hash_value_t*)spri_allocate_type(sizeof(stackref_hash_value_t ));
	
       	sshk->pc=pc;
      	sshv->esp_offset=offset;
       	sshv->size=size;
       	sshv->parent_stackref=NULL;
       	sshv->child_stackref=NULL;
       	sshv->sibling_stackref=NULL;

       	Hashtable_put(stackrefs_hash, sshk, sshv);

	return sshv;
}

stackref_hash_value_t *add_stack_ref_field(stackref_hash_value_t* parent, libIRDB::virtual_offset_t pc, int size, int offset)
{
       	stackref_hash_key_t *sshk=(stackref_hash_key_t*)spri_allocate_type(sizeof(stackref_hash_key_t ));
       	stackref_hash_value_t *sshv=(stackref_hash_value_t*)spri_allocate_type(sizeof(stackref_hash_value_t ));
	
       	sshk->pc=pc;
      	sshv->esp_offset=offset;
       	sshv->size=size;
	sshv->parent_stackref=parent;
       	sshv->child_stackref=NULL;

	/* link into the parent's child list */
       	sshv->sibling_stackref=parent->child_stackref;
	parent->child_stackref=sshv;

	return sshv;
}

