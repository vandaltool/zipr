#include <math.h>
#include <stdio.h>
#include <stdint.h>

int bar()
{
	printf("in bar\n");
}
int foo()
{
	printf("in foo\n");
	foo2();
}

