#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  unsigned long a=0, b=0;
  unsigned long result;
  
  if (argc >= 2)
          a = (unsigned long) strtoul(argv[1], NULL, 10);

  if (argc >= 3)
          b = (unsigned long) strtoul(argv[2], NULL, 10);

  result = a + b;

  printf("a=%lu + b=%lu = %lu\n", a, b, result);

  return 0;
}
