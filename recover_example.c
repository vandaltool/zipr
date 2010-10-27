#include <stdio.h>

void fred(char* ptr) 
{
	scanf("%s", ptr);
}

void bar(char* ptr)
{
	int i;
	for(i=0;i<10;i++)
		fred(ptr);
}

int foo()
{
	char ptr[10];
	bar(ptr);
	return 1;

}

int main()
{
	if(foo())
		printf("Success");

	return 0;
}
