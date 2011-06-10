/*
 * bitvector.h - bit vector headers.
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

/* bitvector.h
 *
 * Author: mc2zk
 * Description:  bit vector implementation using chars
 */

#ifndef bitvector_h
#define bitvector_h

struct bitvector
{
	char * the_bits;
	int size;	/* size of the bit vector - number of bits represented */
	int num_bytes;
};
typedef struct bitvector bitvector_t;


/* 
 * allocate memory for the bitvector given number of 
 * bit fields, number of data chunks 
 */
bitvector_t * allocate_bitvector(int num_fields, int num_data_chunks);

/* reclaim the allocated memory */
void free_bitvector(bitvector_t *the_bitvector_to_be_freed);

/* set the bit to 1 */
void set_bitvector(bitvector_t *the_vector, int the_bit_to_set);

/* clear the bit */
void clear_bit(bitvector_t *the_vector, int the_bit_to_clear);

/* get a bit from the bit vector */
char get_bit(bitvector_t *the_vector, int the_bit);

/* print/dump the bit vector: hexadecimal format */
void print_bitvector_hex(FILE *fout, bitvector_t *the_vector);

/* print/dump the bit vector: binary format */
void print_bitvector_binary(FILE *fout, bitvector_t *the_vector);

/* print the bit vector one bit at a time */
void print_bitvector_bits(FILE *fout, bitvector_t *the_vector);

/* find_bit_number(): Calculate the bit number in the vector to find  */
int find_bit_number(int num_fields, int which_data_chunk, int which_field_to_get);

#endif
