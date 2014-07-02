#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

int main(int argc, char **argv)
{
  unsigned int t;
  unsigned long long i = (unsigned long long) strtoull(argv[1], NULL, 10);

  t = i;
  if (t > 65000)
    printf("too big: trunc = %u\n", t);
  else
    printf("just right: trunc = %u, i = %llu\n", t, i);
}
