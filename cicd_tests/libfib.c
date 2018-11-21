extern int fib_main();
extern int fib2();
extern int fib2p(int n, int (*)(int,int));

int add2(int n1, int n2)
{
	if (n1 == 0)
		return n2;
	else if (n2 == 0)
		return n1;
	else if (n2 == 2)
		return add2(add2(n1, 1),1);
	else
		return add2(0, n2) + add2(n1, 0);
}

int fib(int n)
{
	if (n <= 2)
		return 1;
	else
		return fib_main(n-1) + fib2(n-2);

}

int fibp(int n, int (*addp)(int,int))
{
	if (n <= 2)
		return 1;
	else
		return (*addp)(fib_main(n-1),fib2p(n-2,&add2));
}
