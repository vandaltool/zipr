
char b[100];


extern void memcpy(int,int,int);

main()
{
	char a[100];

	foo(a,b);
	printf("", a);
}

foo(char *c, char* d)
{

	int i;
	int offset=c-d;
	for(i=0;i<100;i++)
	{
		*(d+offset)=*(d);
		d++;
	}


}

