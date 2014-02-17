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

	if (argc < 3)
	{
		fprintf(stderr, "usage: %s <iter> <signatureFile> <queryFile>\n", argv[0]);
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

	int attack_detected = 0;
	int count = atoi(argv[1]);
	int modval = count / 10;
	
	for (i = 0; i < count; ++i)
	{
		if (i % modval == 0) fprintf(stderr," running iteration #%d (%d)\n", i, modval);
		if (!sqlfw_verify_fast(file_contents))
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
