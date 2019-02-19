#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char * argv[])
{
	FILE *filed;
	char *userinput=malloc(20);
	char *outputfile=malloc(20);
	char buf[256];
	
	if (argc != 2)
	{
		printf("Usage: %s <file_to_open>\n", argv[0]);
		exit(0);
	}
	
	// point outputfile at the help file
	strcpy(outputfile, "help.txt");
	strcpy(userinput, argv[1]);

	// let's check out the memory addresses of userinput and outputfile
	printf("userinput @ %p: %s\n",userinput,userinput);
	printf("outputfile @ %p: %s\n", outputfile, outputfile);

	// Do some error-checking: no /etc/passwd allowed as user input
	if (strcmp("/etc/passwd",userinput)==0)
	{
		fprintf(stderr, "ERROR:  You may not specify /etc/passwd as a file to view.\n");
		exit(1);
	}

	filed = fopen(userinput, "r");
	if (filed==NULL)
	{
		// if the file can't be opened, then print the help.txt
		fprintf(stderr, "\nerror opening file %s\n", userinput);
		sprintf(buf, "%s %s", "cat", outputfile);
		system(buf);
		exit(1);
	}
	else
	{
		printf("\nThe contents of %s are:\n", userinput);
		fflush(stdout);
		// print the word count of the file
		sprintf(buf, "%s %s", "cat", userinput);
		system(buf);
	}

	// fprintf(filed, "%s\n", userinput);
	fclose(filed);
	return 0;
}
