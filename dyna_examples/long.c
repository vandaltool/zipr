#include <stdio.h>
#include <stdlib.h>

main(int argc, char* argv[])
{
	if(argc!=2)
	{
		printf("You suckage.\n");
		exit(1);
	}

	while(1)
	{
		printf("Sleeping %d ...\n", atoi(argv[1]));
		sleep(atoi(argv[1]));
	}
}
