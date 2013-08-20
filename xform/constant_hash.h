/*
 * constant_hash.h - hashtable for contants.
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

#ifndef constant_hash_h
#define constant_hash_h


enum constant_hash_type { cht_ESP, cht_EBP, cht_GLOBAL, cht_NUMBER, cht_UNKNOWN };
typedef enum constant_hash_type constant_hash_type_t;
enum constant_hash_field {chf_DISPLACEMENT='d', chf_IMMEDIATE='i', chf_SCALE='s', chf_IMPLICIT='m', chf_OTHER='o'};
typedef enum constant_hash_field constant_hash_field_t;

extern Hashtable *constants_hash;
struct constant_hash_key
{
        int pc;
        int the_const;
	constant_hash_field_t field;
};
typedef struct constant_hash_key constant_hash_key_t;
struct constant_hash_value
{
        constant_hash_type_t type;
	int real_const;
};
typedef struct constant_hash_value constant_hash_value_t;

long constants_compute_hash(void* key1);

long constants_key_compare(void* key1, void* key2);

constant_hash_value_t * add_constant_ref(app_iaddr_t pc,int the_const, constant_hash_field_t the_field, constant_hash_type_t the_type);

constant_hash_value_t * get_constant_ref(app_iaddr_t pc, int the_const, constant_hash_field_t the_field);

const char* constant_hash_type_to_string(constant_hash_type_t type);

#endif
