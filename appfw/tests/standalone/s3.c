#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "appfw.h"
#include "sqlfw.h"
const int EXIT_CODE_ATTACK_DETECTED = 0;
const int EXIT_CODE_NO_ATTACK = 1;
const int EXIT_CODE_USAGE = 2;

int main(int argc, char **argv)
{
	int i = 0;
	if (argc < 2)
	{
		fprintf(stderr, "usage: %s <signatureFile>\n", argv[0]);
		return EXIT_CODE_USAGE;
	}

	// initialize signatures from file
	appfw_init_from_file(argv[1]);
	sqlfw_init();

	while (!feof(stdin))
	{

		char buffer[1024*1024];
		char query[1024*4];
		query[0]=0;
		int length=0;
		while (!feof(stdin))
		{
			char *res=fgets(buffer,1024*4,stdin);
			if (!res) break; //end of file
			if (!strcmp(buffer,"$$\n")) //end of query
				break;
			strcat(query,buffer);
		}
		char * query_structure = malloc(strlen(query)+1 * (sizeof(char)));
		// free(query_structure);
		//we have the query in 'query' now
		int result;
		if ((result = sqlfw_verify_s(query, query_structure)) == S3_SQL_SAFE)
		{
			printf("Safe");
			// fprintf(stderr, "no attack detected\n");
			// return EXIT_CODE_NO_ATTACK;
		}
		else
		{
			if (result & S3_SQL_PARSE_ERROR)
				printf("S3: Parse Error\n");

			if (result & S3_SQL_ERROR)
				printf("S3: Generic Error\n");

			if (result & S3_SQL_ATTACK_DETECTED)
				printf("S3: Attack Detected\n");

			// fprintf(stderr, "attack detected: %s\n", argv[2]);
			// return EXIT_CODE_ATTACK_DETECTED;
		}
		appfw_display_taint("STRUCT", query, query_structure);
		free(query_structure);

		printf("\n$$\n");	
		fflush(stdout);
	}
	return 0;
}
