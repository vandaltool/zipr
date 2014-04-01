#include <stdlib.h>

int main(int argc, char **argv)
{
  long int a = atol(argv[1]);
  long int b = atol(argv[2]);
  long int d = a * b;

  printf("%ld * %ld = %ld\n", a, b, d);

  return 0;
}
