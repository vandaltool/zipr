#include <stdio.h>
#include "appfw.h"
#include "sqlfw.h"

const int EXIT_CODE_ATTACK_DETECTED = 0;
const int EXIT_CODE_NO_ATTACK = 1;
const int EXIT_CODE_USAGE = 2;

int main(int argc, char **argv)
{
	char *filename = NULL;
	char *errorMessage = NULL;

	if (argc < 3)
	{
		fprintf(stderr, "usage: %s <signatureFile> <queryString>\n", argv[0]);
		return EXIT_CODE_USAGE;
	}

	// initialize signatures from file
	appfw_init_from_file(argv[1]);
	sqlfw_init();

	if (sqlfw_verify(argv[2], &errorMessage))
	{
		fprintf(stderr, "no attack detected\n");
		return EXIT_CODE_NO_ATTACK;
	}
	else
	{
		fprintf(stderr, "attack detected: %s\n", argv[2]);
		return EXIT_CODE_ATTACK_DETECTED;
	}
}
