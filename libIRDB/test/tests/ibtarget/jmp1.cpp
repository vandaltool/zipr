#include <stdlib.h>
#include <stdio.h>

int add(int a, int b)
{
	int x = a + b;
	printf("add(): %d + %d = %d\n", a, b, x);
	return x;
}

int main(int argc, char **argv)
{
	int i = atoi(argv[1]);

	switch(i) 
	{
		case 20:
			printf("i = 20\n");
			break;
		case 22:
			printf("i = 22\n");
			break;
		case 23:
			printf("hello: ");
			printf("i = 23\n");
			break;
		case 24:
			printf("i = 24\n");
			break;
		case 25:
			printf("i = 25\n");
			break;
		case 26:
			printf("bar: ");
			printf("i = 26\n");
			break;
		case 27:
			printf("i = 27\n");
			break;
		case 28:
			printf("i = 28\n");
			break;
		default:
			printf("i = %d\n", i);
	}

	int val = add(i,i);
	printf("main(): %d + %d = %d\n", i, i, val);
}
