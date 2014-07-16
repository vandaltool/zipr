#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

int main(int argc, char **argv)
{
  short t;
  unsigned long long i = (unsigned long long) strtoull(argv[1], NULL, 10);

  t = i;
  if (t > 256)
    printf("too big: trunc = %d\n", t);
  else
    printf("just right: trunc = %d, i = %ld\n", t, i);
}
