#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

int main(int argc, char **argv)
{
  char c;
  unsigned long long i = (unsigned long long) strtoull(argv[1], NULL, 10);

  c = i;
  printf("just right: c = %d, i = %lld\n", c, i);
}
