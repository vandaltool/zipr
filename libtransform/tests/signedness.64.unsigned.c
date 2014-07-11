#include <stdlib.h>
#include <stdio.h>

int main (int argc, char **argv) 
{
	char *foobar = NULL;
	int i;

	i = atoi(argv[1]);

	foobar = malloc(i);

	printf("i = %d\n", i);

	strncpy(foobar, "yeah whatever", i);

	printf("%d: %s\n", i, foobar);

	return 0;
}
