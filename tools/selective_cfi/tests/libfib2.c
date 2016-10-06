extern int fib_main();
extern int fib();

int fib2(n)
{
	if (n <= 1)
		return 1;
	else
//		return fib_main(n-1) + fib2(n-2);
		return fib2(n-1) + fib_main(n-2);
}
