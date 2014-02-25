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
	if (argc < 4)
	{
		fprintf(stderr, "usage: %s #iter <signatureFile> <queryStructureCacheFile>\n", argv[0]);
		return EXIT_CODE_USAGE;
	}

	// initialize signatures from file
	int iter = atoi(argv[1]);

struct timeval t1, t2;
struct timeval tt1, tt2;

	gettimeofday(&t1, NULL);

	appfw_init_from_file(argv[2]);
	sqlfw_init_from_file(argv[3]);

	gettimeofday(&t2, NULL);

	fprintf(stderr, "init: elapsed(msec): %f\n", ((t2.tv_sec - t1.tv_sec) * 1000000.0 + (t2.tv_usec - t1.tv_usec)) / 1000.0 );

	gettimeofday(&t1, NULL);
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
		// free(query_structure);
		//we have the query in 'query' now
		for (i = 0; i < iter; ++i)
		{
		char * query_structure = malloc(strlen(query)+1 * (sizeof(char)));
		int result= sqlfw_verify_s(query, query_structure);
/*
fprintf(stderr, "iter #%d, query[%s]\n", i, query);
		appfw_display_taint("STRUCT", query, query_structure);
*/
		if (sqlfw_is_safe(result))
		{
			printf("Safe");
			// fprintf(stderr, "no attack detected\n");
			// return EXIT_CODE_NO_ATTACK;
		}
		else
		{
			if (sqlfw_is_parse_error(result))
				printf("S3: Parse Error\n");

			if (sqlfw_is_attack(result))
				printf("S3: Attack Detected\n");

			// fprintf(stderr, "attack detected: %s\n", argv[2]);
			// return EXIT_CODE_ATTACK_DETECTED;
		}
//		appfw_display_taint("STRUCT", query, query_structure);
		free(query_structure);
//gettimeofday(&tt2, NULL);
		}

		printf("\n$$\n");	
		fflush(stdout);
	}
	gettimeofday(&t2, NULL);

	fprintf(stderr, "total: elapsed(msec): %f\n", ((t2.tv_sec - t1.tv_sec) * 1000000.0 + (t2.tv_usec - t1.tv_usec)) / 1000.0 / iter);

	sqlfw_save_query_structure_cache("sql.structure.cache");
	return 0;
}
