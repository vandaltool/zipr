// Test of doubleing point overflow

/*
@GOOD_ARGS 123.0 1.0
@NORMAL_OUTPUT_CONTAINS 1
@BAD_ARGS 123.0 119.0
@ATTACK_SUCCEEDED_OUTPUT_CONTAINS 0
*/

#include <stdlib.h>
#include <stdio.h>

int equals(double x, double y)
{
  return (x == y);
}


int main(int argc, char **argv)
{
  if (argc <= 2)
  {
    printf ("Usage: prog N divisor\n");
    exit(2);
  }

  double x = atof(argv[1]);
  double y = x;

  double z = atof(argv[2]);

  y /= z;
  y *= z;

  printf("%d\n", equals(x,y));
  exit(0);
}
