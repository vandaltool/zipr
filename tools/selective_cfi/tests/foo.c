#include <stdlib.h>
#include <stdio.h>

extern int foo();

int main()
{
	foo();
}

int foo2()
{
	printf("In foo2\n");
}
