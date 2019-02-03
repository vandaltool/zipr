#include <stdio.h>

extern int fib();
extern int fib2();
extern int fibp(int n, int (*)(int,int));

int add(int n1, int n2)
{
	if (n1 == 0)
		return n2;
	else if (n2 == 0)
		return n1;
	else if (n1 == 2)
		return add(1, add(1, n2));
	else
		return n1 + add(0, n2);
}

main(int argc, char **argv)
{
	int x = 3;
	int f = 0;
	if (argc >= 2)
		x = atoi(argv[1]);

	if (x <= 2)
		f = fib_main(x);
	if (x == 3)
		f = fib2(x);
	else if (x == 4)
		f = fib_simple(x);
	else if (x == 5)
		f = fibp(x, &add);
	else
		f = fib(x);
	printf("Fibonacci(%d) = %d\n", x, f);

	return f;
}

fib_main(int f) {
	if (f <= 2)
		return 1;
	else
		return fib_simple(f-1) + fib_main(f-2);
}

fib_simple(int f) {
	if (f <= 2)
		return 1;
	else
		return fib_simple(f-1) + fib_simple(f-2);
}
