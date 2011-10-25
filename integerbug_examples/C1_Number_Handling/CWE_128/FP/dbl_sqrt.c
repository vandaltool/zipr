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

double root(double x)
{
  return sqrt(x);
}


int main(int argc, char **argv)
{
  double x = atof(argv[1]);
  double y = root(x);
  printf("%f\n", y);
  exit(0);
}
