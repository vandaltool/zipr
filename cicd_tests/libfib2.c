extern int fib_main();
extern int fib();

int fib2(int n)
{
	if (n <= 2)
		return 1;
	else
		return fib2(n-1) + fib_main(n-2);
}

int fib2p(int n, int (*addp)(int,int))
{
	if (n <= 2)
		return (*addp)(1,0);
	else
		return (*addp)(fib2(n-1),fib2p(n-2, addp));
}
