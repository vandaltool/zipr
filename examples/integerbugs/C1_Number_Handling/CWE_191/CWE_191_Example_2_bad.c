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
@BAD_ARGS 0
@NORMAL_OUTPUT_CONTAINS N = 49
@ATTACK_SUCCEEDED_OUTPUT_CONTAINS N = 429
// bjm removed for grace TTACK_SUCCEEDED_CODE 1

*/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#ifdef ASSERT
  #include <assert.h> 
#endif

main (int argc, char **argv)
{
  if (argc < 2) exit(2);
  unsigned int j = atoi(argv[1]);
  j = j - 1;

#ifdef ASSERT
  assert(isdigit(argv[1][0]));  
  assert(atoi(argv[1])>0);
#endif

  printf("N = %u\n", j);
  exit(0);
}
   
