// Test of doubleing point overflow

/*
@GOOD_ARGS 10
@NORMAL_OUTPUT_CONTAINS e\+02
@BAD_ARGS 1e200
@ATTACK_SUCCEEDED_OUTPUT_CONTAINS inf
*/

#include <stdlib.h>
#include <stdio.h>

double square(double x)
{
  return x * x;
}


int main(int argc, char **argv)
{
  double x = atof(argv[1]);
  double y = square(x);
  printf("%e\n", y);
  exit(0);
}
