// Test of floating point overflow

/*
@GOOD_ARGS 10
@NORMAL_OUTPUT_CONTAINS e\+02
@BAD_ARGS 1e20
@ATTACK_SUCCEEDED_OUTPUT_CONTAINS inf
*/

#include <stdlib.h>
#include <stdio.h>

float square(float x)
{
  return x * x;
}


int main(int argc, char **argv)
{
  float x = atof(argv[1]);
  float y = square(x);
  printf("%e\n", y);
  exit(0);
}
