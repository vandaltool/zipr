#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

int main(int argc, char **argv)
{
  unsigned char c;
  unsigned long long i = (unsigned long long) strtoull(argv[1], NULL, 10);

  c = i;
  if (c > 128)
    printf("too big\n");
  else
    printf("just right: c = %u, i = %lld\n", c, i);
}
