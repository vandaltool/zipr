// Test of floating point overflow


/*
@GOOD_ARGS 123.0 1.0
@NORMAL_OUTPUT_CONTAINS 1
@BAD_ARGS 1231223.0123123 119.01123123
@ATTACK_SUCCEEDED_OUTPUT_CONTAINS 0
*/

#include <stdlib.h>
#include <stdio.h>

int equals(float x, float y)
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

  float x = atof(argv[1]);
  float y = x;

  float z = atof(argv[2]);

  y /= z;
  y += 100000;
  y *= z;
  y -= 100000;

  printf("%d\n", equals(x,y));
  exit(0);
}
