#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

int main(int argc, char **argv)
{
  short s;
  unsigned long long i = (unsigned long long) strtoull(argv[1], NULL, 10);

  s = i;
  printf("just right: s = %d, i = %lld\n", s, i);
}
