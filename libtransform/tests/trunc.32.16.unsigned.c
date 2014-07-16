#include <stdio.h>

int main(int argc, char **argv)
{
  unsigned short s;
  int i;

  i = atoi(argv[1]);
  s = i;
  if (s > 256)
    printf("whatever\n");
  printf("s = %d\n", s);
}
