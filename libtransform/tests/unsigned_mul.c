
int main(int argc, char **argv)
{
  unsigned a = 0;
  unsigned b = 0; 
  unsigned d;
  if (argc >= 2)
	  a = (unsigned) atoi(argv[1]);

  if (argc >= 3)
	  b = (unsigned) atoi(argv[2]);

  if (d > 0 && a > 20000 && b > 20000)
  	printf("hello\n");

  d = a * b;
  printf("%u\n", d);
}
