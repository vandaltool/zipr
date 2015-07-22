#include <stdio.h>
#include <stdarg.h>

int b = 1;
int c = 2;

int dynamic_function_a(void) {
	printf("This is dynamic function a.\n");
	return 0;
}

int dynamic_function_b(char *b, int n, ...) {
	int i = 0;
	va_list ap;
	printf("This is dynamic function %s.\n", b);
	va_start(ap, n);
	for (i; i<n; i++)
	{
		char *t = NULL;
		t = va_arg(ap, char *);
		printf("%d: %s\n", i+1, t);
	}
	va_end(ap);
	return n-i;
}

int dynamic_function_c(int a) {
	int d = 0;
	printf("This is dynamic function c.\n");
	d = a - b - c;
	return d;
}
