/*
 * Copyright (c) 2013, 2014 - University of Virginia 
 *
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

#include <stdio.h>
#include "appfw.h"
#include "sqlfw.h"
#include <stdlib.h>
const int EXIT_CODE_ATTACK_DETECTED = 0;
const int EXIT_CODE_NO_ATTACK = 1;
const int EXIT_CODE_USAGE = 2;

int main(int argc, char **argv)
{
	char *filename = NULL;
	char *errorMessage = NULL;
	char *query_structure;
	int i;

	if (argc < 4)
	{
		fprintf(stderr, "usage: %s <iterations> <signatureFile> <queryFile>\n", argv[0]);
		return EXIT_CODE_USAGE;
	}

	// initialize signatures from file
	appfw_init_from_file(argv[2]);
	sqlfw_init();
	char *file_contents;
	long input_file_size;
	FILE *input_file = fopen(argv[3], "rb");
	fseek(input_file, 0, SEEK_END);
	input_file_size = ftell(input_file);
	rewind(input_file);
	file_contents = malloc((input_file_size+1) * (sizeof(char)));
	fread(file_contents, sizeof(char), input_file_size, input_file);
	fclose(input_file);
	file_contents[input_file_size]=0;

	// get query structure
	query_structure = malloc((input_file_size+1) * (sizeof(char)));
	sqlfw_get_structure(file_contents, query_structure);
	// Abbas, you can do whatever you want with the query structure here
	// Basically only pay attention to the characters marked 'c'
	appfw_display_taint("query structure", file_contents, query_structure);

	// this is the SLOW VERSION
	int attack_detected = 0;
	for (i = 0; i < atoi(argv[1]); ++i)
	{
		if (!sqlfw_verify(file_contents, &errorMessage))
		{
			attack_detected = 1;
		}
	}

	if (attack_detected)
		fprintf(stderr,"attack detected\n");
	else
		fprintf(stderr,"no attack detected\n");

	free(query_structure);
	return attack_detected;
}
