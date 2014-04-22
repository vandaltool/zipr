#include <stdlib.h>

int main(int argc, char **argv)
{
  int a = atoi(argv[1]);
  int b = atoi(argv[2]);
  int d = a * b;

  printf("%d * %d = %d\n", a, b, d);

  return 0;
}
