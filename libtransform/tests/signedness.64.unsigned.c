#include <stdlib.h>
#include <stdio.h>

int main (int argc, char **argv) 
{
	char *foobar = NULL;
	int i, j;

	i = atoi(argv[1]);

	printf("i = %d\n", i);

	foobar = malloc(i); // sign/unsigned error here

	if (!foobar)
	{
		printf("malloc failed\n");
		return 1;
	}

	printf("i = %d\n", i);

	// need the strncpy to induce SIGNEDNESS annotation
	bzero(foobar, i);
	strncpy(foobar, "yeah whatever", i-1);
	printf("%d: %s\n", i, foobar);

	return 0;
}
