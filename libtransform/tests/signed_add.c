#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

int main(int argc, char **argv)
{
  int64_t a = atol(argv[1]);
  int64_t b = atol(argv[2]);
  volatile int64_t result;
  
  printf("a = %ld; b = %ld\n", a, b);

  result = 1 + a + b;

  printf("1 + a=%ld + b=%ld = %ld\n", a, b, result);

  return 0;
}
