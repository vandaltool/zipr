#include <stdio.h>

int main(int argc, char **argv)
{
  char c;
  short s;

//  s = atoi(argv[1]);
  s = 10000;
  c = s++;
  if (c > 128 || s > 128)
    printf("too big\n");
  else
    printf("just right: s = %d, c = %d\n", s, c);
}
