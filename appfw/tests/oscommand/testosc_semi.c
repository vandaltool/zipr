#include <stdio.h>
#include <stdlib.h>

char *foobar="ls";
char *foobar3="%s;%s";

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
	else
	{
		fprintf(stderr,"data: %s\n", data);
	}

	char cmd[2048];
	sprintf(cmd, "  echo hello; %s\n", data);
	fprintf(stderr," about to issue command: %s\n", cmd);
	system(cmd);
}
