#include <stdlib.h>
#include <stdio.h>

int main (int argc, char **argv) 
{
	char *foobar = NULL;
	int i;

	i = atoi(argv[1]);
	foobar = malloc(i); 
	strncpy(foobar, "yeah whatever", i);

	printf("%s\n", foobar);

	return 0;
}
