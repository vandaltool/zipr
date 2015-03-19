#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int foo(char *text)
{
	fprintf(stderr, "foo called\n");
	return strlen(text);	
}

int bar(char *text)
{
	fprintf(stderr, "bar called\n");
	return strlen(text) + 1000;	
}

struct fptrs {
	char buf[8];
	int (*fptr)(char *);
};

int main(int argc, char **argv)
{
	struct fptrs yo;
	int len = strlen(argv[1]);
	int i;

	fprintf(stderr, "foo = 0x%x  bar = 0x%x\n", &foo, &bar);

	// loop 1000 times
	for (i = 0; i < 1000; ++i)
	{
		yo.fptr = &foo;

		if (i % 2 == 0)
			yo.fptr = &bar;
	
		memcpy(&yo.buf, argv[1], len);
		fprintf(stderr, "fptr = 0x%x\n", yo.fptr);

		int value = (*yo.fptr)(argv[1]);
		fprintf(stderr, "value = %d\n", value);
	}

	return 0;
}
