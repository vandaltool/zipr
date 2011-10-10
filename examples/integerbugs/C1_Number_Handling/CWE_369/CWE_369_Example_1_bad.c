/*
Divide By Zero

Description Summary
The product divides a value by zero. 

Extended Description
This weakness typically occurs when an unexpected value is provided to the
product, or if an error occurs that is not properly detected. It frequently
occurs in calculations involving physical dimensions such as size, length,
width, and height. 

Example 3
The following C# example contains a function that divides two numeric values
without verifying that the input value used as the denominator is not zero.
This will create an error for attempting to divide by zero, if this error is
not caught by the error handling capabilities of the language, unexpected
results can occur.
(Bad Code)Example Language: C

@BAD_ARGS 17 0
@GOOD_ARGS 5 5
@ATTACK_SUCCEEDED_CODE 136

*/

#include <stdio.h>
#include <stdlib.h>
#ifdef ASSERT
  #include <assert.h> 
#endif

int Division(int x, int y)
{
  return (x / y);
}

int main(int argc, char **argv)
{
  if (argc < 3) exit(2);
  int a = atoi(argv[1]);
  int b = atoi(argv[2]);
  int c = Division(a, b);
#ifdef ASSERT
  assert(b != 0);
#endif

  exit(0);
}
