#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

int main(int argc, char **argv)
{
  unsigned int s;
  unsigned long long i = (unsigned long long) strtoull(argv[1], NULL, 10);

  s = i;
  if (s > 256)
    printf("too big\n");
  else
    printf("just right: s = %u, i = %lld\n", s, i);
}
