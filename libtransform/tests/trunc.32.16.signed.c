#include <stdio.h>

int main(int argc, char **argv)
{
  short s;
  int i;

  i = atoi(argv[1]);
  s = i;
  if (s > 256)
    printf("too big s = %d\n", s);
  else
    printf("just right: s = %d\n", s);
}
