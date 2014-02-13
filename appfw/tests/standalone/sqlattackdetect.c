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

	if (argc < 3)
	{
		fprintf(stderr, "usage: %s <signatureFile> <queryFile>\n", argv[0]);
		return EXIT_CODE_USAGE;
	}

	// initialize signatures from file
	appfw_init_from_file(argv[1]);
	sqlfw_init();
	char *file_contents;
	long input_file_size;
	FILE *input_file = fopen(argv[2], "rb");
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

	if (sqlfw_verify(file_contents, &errorMessage))
	{
		fprintf(stderr, "no attack detected\n");
		return EXIT_CODE_NO_ATTACK;
	}
	else
	{
		fprintf(stderr, "attack detected: %s\n", file_contents);
		return EXIT_CODE_ATTACK_DETECTED;
	}

	free(query_structure);

}
