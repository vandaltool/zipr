/*
Integer Underflow (Wrap or Wraparound)

Description Summary
The product subtracts one value from another, such that the result is less than the minimum allowable integer value, which produces a value that is not equal to the correct result. 

Extended Description
This can happen in signed and unsigned cases. 

Example 1
The following example has an integer underflow. The value of i is already at the lowest negative value possible. The new value of i is 2147483647.
(Bad Code)Example Language: C 

@GOOD_ARGS 50
@BAD_ARGS -2147483648
@NORMAL_OUTPUT_CONTAINS N = 49
@ATTACK_SUCCEEDED_OUTPUT_CONTAINS N = 214

// bjm remove exit   TTACK_SUCCEEDED_CODE 1

*/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#ifdef ASSERT
  #include <assert.h>
#endif

main (int argc, char ** argv)
{
  if (argc < 2) exit(2);
  int i = atoi(argv[1]);

  i = i - 1;
#ifdef ASSERT
assert(atoi(argv[1])>INT_MIN);
#endif
  printf("N = %d\n", i);
  exit(0);
}
   
