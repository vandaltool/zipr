
#include <stdio.h>
#include <math.h>

main(int argc, char* argv[])
{
	if(argc<3)
	{
		printf("Need two parameters...\n");
		return 1;
	}

	int i=0;
	for(i=0; i<3; i++)
	{
		printf("%s ^ %s = %f\n", argv[1], argv[2], pow(atoi(argv[1]), atoi(argv[2])));
		printf("%s ^ %s = %f\n", argv[1], argv[2], pow(atoi(argv[1]), atoi(argv[2])));
	}
}
