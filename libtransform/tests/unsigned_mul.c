#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  unsigned long a = 0;
  unsigned long b = 0; 
  unsigned long d;

  if (argc >= 2)
	  a = (unsigned long) strtoul(argv[1], NULL, 10);

  if (argc >= 3)
	  b = (unsigned long) strtoul(argv[2], NULL, 10);

  d = a * b;
  printf("%lu * %lu = %lu\n", a, b, d);

  return 0;
}
