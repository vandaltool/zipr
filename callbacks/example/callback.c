

__attribute__ ((externally_visible)) __attribute__ ((used)) print_hello()
{
	char str[]="Hello";
	write(1,str,sizeof(str));
}
