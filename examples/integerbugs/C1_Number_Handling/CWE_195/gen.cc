#include <stdio.h>
#include <stdlib.h>

/*
This is used for generating test data and not an actual test
@DRIVER
*/

int main(int argc, char **argv)
{
  if (argc < 3) {printf("Usage: gen <reported size> <actual size>\n"); exit(2); }
  short n = atoi(argv[1]);
  int m = atoi(argv[2]);
  char *c = (char *)&n;

  printf ("%c%c", c[0], c[1]);
  int i;
  for (i = 0; i < m; ++i) printf("x");
}
  

