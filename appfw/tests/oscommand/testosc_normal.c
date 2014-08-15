#include <stdio.h>
#include <stdlib.h>

char *a = "ls";
char *b = "-lt";

/* 
   Pass in: ls -lt           test should exit normally
   Pass in: cat Makefile     OS Command Injection should be detected
*/
int main(int argc, char **argv)
{
	char *data = getenv("DATA");

	if (data)
	{
		fprintf(stderr,"data: %s\n", data);
		char cmd[2048];
		sprintf(cmd,"%s", data);
		system(cmd);
	}
	else
	{
		fprintf(stderr,"DATA env. var not set\n");
		exit(1);
	}
}
