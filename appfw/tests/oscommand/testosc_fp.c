#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	char *data = getenv("DATA");

	if (data)
	{
		fprintf(stderr,"data: %s\n", data);
		char cmd[2048];
		sprintf(cmd,"cat Makefile; %s", data);
		system(cmd);
	}
	else
	{
		fprintf(stderr,"DATA env. var not set\n");
		exit(1);
	}
}
