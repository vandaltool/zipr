#include <stdio.h>

int main(int argc, char **argv)
{
  char s;
  int i;

  i = atoi(argv[1]);
  s = i;
  if (s > 128)
    printf("too big\n");
  else
    printf("just right: i = %d, s = %d\n", i, s);
}
