#include <stdio.h>

extern int fib();
extern int fib2();

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
	else
		f = fib(x);
	printf("Fibonacci(%d) = %d\n", x, f);
}

fib_main(int f) {
	if (f <= 1)
		return 1;
	else
		return fib_simple(f-1) + fib_main(f-2);
}

fib_simple(int f) {
	if (f <= 1)
		return 1;
	else
		return fib_simple(f-1) + fib_simple(f-2);
}
