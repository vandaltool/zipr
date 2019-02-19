/*
 * bitvector.c - bit vector routines.
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

/* bitvector.c
 *
 * Author: mc2zk
 * Description:  bit vector implementation using chars
 */


#include "meds_all.h"


/* 
 * allocate memory for the bitvector given number of 
 * bit fields, number of data chunks 
 */
bitvector_t * allocate_bitvector(int num_fields, int num_data_chunks)
{
	/* round number of bits up to 8 then convert to bytes */
	int num_bytes_to_allocate=(((num_fields*num_data_chunks) + 7) &~7)/8;

	bitvector_t * the_bitvector = (bitvector_t*)spri_allocate_type(sizeof(bitvector_t));
	/* allocate the_bits 
	 *  	the number of bits needed is num_fields * num_data_chunks 
	 * 	rounded to nearest 8 (num bits in a char)
	 * 	to get bytes, need to divide by 8 bits
 	 */
	the_bitvector->the_bits = (char*)spri_allocate_type(num_bytes_to_allocate*sizeof(char));
	the_bitvector->size = num_fields*num_data_chunks;
	the_bitvector->num_bytes = num_bytes_to_allocate;

	return the_bitvector;
}

/* reclaim the allocated memory */
void free_bitvector(bitvector_t *the_bitvector_to_be_freed)
{
#ifndef NDEBUG
// 	STRATA_LOG("profile_fields_allocate","bitvector address:  0x%x size: %d\n", the_bitvector_to_be_freed, the_bitvector_to_be_freed->size);
#endif
	/* first free the_bits vector */
	spri_deallocate_type((void *)the_bitvector_to_be_freed->the_bits, ((the_bitvector_to_be_freed->size+7)&~7)/8);

	/* then free the bitvector_t memory */
	spri_deallocate_type((void *)the_bitvector_to_be_freed, sizeof(bitvector_t));

}

/* set the bit to 1 */
void set_bitvector(bitvector_t *the_vector, int the_bit_to_set){

	/* check to make sure that the bounds are not being exceeded */
	if(the_vector &&(the_bit_to_set >= the_vector->size))
	{
		/* don't set the bit if out of bounds */
		return;
	}
	the_vector->the_bits[the_bit_to_set >> 3] |= 1 << (the_bit_to_set & 7);
}

/* clear the bit */
void clear_bit(bitvector_t *the_vector, int the_bit_to_clear){
	the_vector->the_bits[the_bit_to_clear >> 3] &= ~(1 << (the_bit_to_clear & 7));
}


/* get a bit from the bit vector: returns 1 if bit is 1, 0 if bit is 0 */
char get_bit(bitvector_t *the_vector, int the_bit){
	return (the_vector->the_bits[the_bit >> 3] & (1 << (the_bit & 7))) !=0;

}

/* function to print the bit vector in hexadecimal */
void print_bitvector_hex(FILE * fout, bitvector_t *the_vector)
{
	int i=0;
	int num_elems_in_bytes=the_vector->num_bytes;

	/* how many elements should be in the int array? */
	int num_elems_in_int=(num_elems_in_bytes+3)/4 /* bytes per int */;

	/* print out the number of elements in the integer array */
	fprintf(fout, "%d %d ", the_vector->size, num_elems_in_int);

	/* convert to int array */
	/* iterate through bitvector and populate intArray */
	for(i=0; i<num_elems_in_bytes; i+=sizeof(int))
	{
		fprintf(fout, "%x ", *(int*)(&the_vector->the_bits[i]));
	}

	fprintf(fout, "\n");
}

/* print/dump the bit vector in binary */
void print_bitvector_binary(FILE * fout, bitvector_t *the_vector)
{
	fprintf(fout, "%d ", the_vector->size /* size in bits */);

	/* print out the bits */
	fwrite(the_vector->the_bits, sizeof(char), the_vector->num_bytes, fout);

	fprintf(fout, "\n");
}

/* print/dump the bit vector one bit at a time for debug purposes */
void print_bitvector_bits(FILE * fout, bitvector_t *the_vector)
{
	int i=0;
	fprintf(fout, "size in bits: %d \n", the_vector->size /* size in bits */);

	/* print out the bits */
	for(i=0; i < the_vector->size; i++)
	{
		fprintf(fout, "%d ", get_bit(the_vector, i) );
	}

	fprintf(fout, "\n");
}


/* find_bit_number(): Calculate the bit number in the vector to find  */
int find_bit_number(int num_fields, int which_data_chunk, int which_field_to_get
)
{
	int the_bit_number=(which_data_chunk*num_fields)+which_field_to_get;

	return the_bit_number;
}

