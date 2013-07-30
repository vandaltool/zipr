#include <stdio.h>

char *foobar="ls";
char *foobar3="%s;%s";
char *foobar5="%s-foo%s";

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

	char cmd[2048];
	sprintf(cmd, "echo hello; ls -l%s --exec %s; find -exec ls %s\n", data,data,data);
	system(cmd);
}
