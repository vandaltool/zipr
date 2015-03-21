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

	switch(val) 
	{
		case 20:
			val = 35;
			printf("a\n");
			break;
		case 22:
			val = add(21,31);
			printf("36\n");
			break;
		case 23:
			val = add(val, val);
			printf("37\n");
			break;
		case 24:
			val = add(val, 34);
			printf("38\n");
			break;
		case 25:
			val = 38;
			printf("39\n");
			break;
		case 26:
			val = 45;
			printf("45\n");
			break;
		case 27:
			val = 55;
			printf("55\n");
			break;
		case 28:
			val = 69;
			printf("69\n");
			break;
		case 29:
			val = 72;
			break;
		case 31:
			val = 82;
			printf("82\n");
			break;
		default:
			val = 99;
	}

	printf("val = %d\n", val);

}
