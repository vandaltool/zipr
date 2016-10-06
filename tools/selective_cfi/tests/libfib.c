extern int fib_main();
extern int fib2();

int fib(n)
{
	if (n <= 1)
		return 1;
	else
		return fib_main(n-1) + fib2(n-2);

}
