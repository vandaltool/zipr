#include <stdlib.h>

int main(int argc, char **argv)
{
  unsigned int a = strtoul(argv[1], NULL, 10);
  unsigned int b = strtoul(argv[2], NULL, 10);
  unsigned int d = a * b;

/*
  if (a < 5 || a < b || d > 27 + a)
    printf("force unsigned annotation\n");
*/

  printf("%u * %u = %u\n", a, b, d);

  return 0;
}
