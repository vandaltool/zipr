// Test of Sign problems

/*
@GOOD_ARGS 100.0
@NORMAL_OUTPUT_CONTAINS 10
@BAD_ARGS -100
@ATTACK_SUCCEEDED_OUTPUT_CONTAINS nan
*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

float root(float x)
{
  return sqrtf(x);
}


int main(int argc, char **argv)
{
  float x = atof(argv[1]);
  float y = root(x);
  printf("%f\n", y);
  exit(0);
}
