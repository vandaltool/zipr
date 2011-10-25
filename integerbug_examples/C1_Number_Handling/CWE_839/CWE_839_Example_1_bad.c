/*
Description Summary
The program checks a value to ensure that it does not exceed a maximum, but it does not verify that the value exceeds the minimum. 

Extended Description
Some programs use signed integers or floats even when their values are only expected to be positive or 0. An input validation check might assume that the value is positive, and only check for the maximum value. If the value is negative, but the code assumes that the value is positive, this can produce an error. The error may have security consequences if the negative value is used for memory allocation, array access, buffer access, etc. Ultimately, the error could lead to a buffer overflow or other type of memory corruption. 

The use of a negative number in a positive-only context could have security implications for other types of resources. For example, a shopping cart might check that the user is not requesting more than 10 items, but a request for -3 items could cause the application to calculate a negative price and credit the attacker's account. 

@GOOD_ARGS 35
@NORMAL_OUTPUT_CONTAINS n is between
@BAD_ARGS -35
@ATTACK_SUCCEEDED_OUTPUT_NOT_CONTAINS n is NOT between

*/ 

#include <stdio.h>
#include <stdlib.h>

int ok(int n)
{
  return (n < 50);
}

int main(int argc, char **argv)
{
  if (ok(atoi(argv[1])))
  {
    printf("n is between 0 and 50\n");
  }
  else
  {
    printf("n is NOT between 0 and 50\n");
  }
  exit(0);
}
