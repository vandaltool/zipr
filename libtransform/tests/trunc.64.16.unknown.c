#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

int main(int argc, char **argv)
{
  short t;
  unsigned long long i = (unsigned long long) strtoull(argv[1], NULL, 10);

  t = i;
  printf("just right: trunc = %d, i = %lld\n", t, i);
}
