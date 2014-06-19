#include <stdio.h>

int main(int argc, char **argv)
{
  unsigned short s;
  unsigned long i;

  i = (unsigned long) strtoul(argv[1], NULL, 10);
  s = i;
  if (s > 256)
    printf("too big\n");
  else
    printf("just right: s = %d\n", s);
}
