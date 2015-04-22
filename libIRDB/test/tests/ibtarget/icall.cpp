#include <stdio.h>
#include <stdlib.h>

int add(int a, int b)
{
	printf("%d + %d = %d\n", a, b, a+b);
	return a + b;
}

int sub(int a, int b)
{
	printf("%d - %d = %d\n", a, b, a-b);
	return a - b;
}

int mul(int a, int b)
{
	printf("%d * %d = %d\n", a, b, a*b);
	return a * b;
}

int main(int argc, char**argv)
{
	int (*fn)(int,int);

	if (argc % 2)
		fn = &add;
	else
		fn = &sub;

	int result = (*fn)(argc, argc+1);
	printf("result = %d\n", result);
}
