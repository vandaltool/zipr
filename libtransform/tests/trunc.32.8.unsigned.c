#include <stdio.h>

int main(int argc, char **argv)
{
  unsigned char c;
  int i;

  i = atoi(argv[1]);
  c = i;
  if (c > 128)
    printf("too big\n");
  else
    printf("just right: c = %d\n", c);
}
