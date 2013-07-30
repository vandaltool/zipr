#include <stdio.h>

char *foobar="ls";
char *foobar3="%s-lt%s";
char *foobar5="%sar%s";

int main(int argc, char **argv)
{
	char *foobar2="ls";
	char *foobar3="%s;%s";
	char *data = getenv("DATA");

	if (!data)
	{
		fprintf(stderr,"DATA env. var not set\n");
		exit(1);
	}

	char *args[2];
	args[0] = malloc(1024);
	args[1] = NULL;
	sprintf(args[0],data);
	execv("/bin/ls", args);
}
