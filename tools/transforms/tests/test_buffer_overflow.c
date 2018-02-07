#include <stdio.h>
#include <string.h>

#define BUFSIZE 32

void do_overflow(char *s)
{
	char tmp[BUFSIZE];
	strcpy(tmp, s);
	printf("target string is: %s\n", tmp);
}

int main(int argc, char **argv)
{
	if (argc > 1)
		do_overflow(argv[1]);
	else
		printf("specify long string on command line as argument to overflow (bufsize=%d)\n", BUFSIZE);
}
