/*
Numeric Truncation Error

Description Summary
Truncation errors occur when a primitive is cast to a primitive of a smaller
size and data is lost in the conversion. 

Extended Description
When a primitive is cast to a smaller primitive, the high order bits of the
large value are lost in the conversion, potentially resulting in an unexpected
value that is not equal to the original value. This value may be required as an
index into a buffer, a loop iterator, or simply necessary state data. In any
case, the value cannot be trusted and the system will be in an undefined state.
While this method may be employed viably to isolate the low bits of a value,
this usage is rare, and truncation usually implies that an implementation error
has occurred. 

Common Consequences
Scope Effect 
Integrity The true value of the data is lost and corrupted data is used.
 
Example 1
This example, while not exploitable, shows the possible mangling of values
associated with truncation errors:
(Bad Code)Example Language: C 

@BAD_ARGS 2147483647 
@GOOD_ARGS 22  
@ATTACK_SUCCEEDED_CODE 1

*/

#include <stdlib.h>
#include <stdio.h>
#ifdef ASSERT
  #include <assert.h>
#endif

int main(int argc, char **argv)
{
  int intPrimitive;
  short shortPrimitive;
if(argc != 2 ){
  fprintf(stderr, "usage: %s ipaddres\n", argv[0]);  
  exit(1);
}
  intPrimitive = atoi(argv[1]);
/*
BJM The intial example had this code.  I changed it to take a command line arg
This will let there be more bad values.  The intened value from this code is
now the Bad_ARGS

  intPrimitive = (int)(~((int)0) ^ (1 << (sizeof(int)*8-1)));
*/
  shortPrimitive = intPrimitive;
  printf("Int: %d\nShort: %d\n", intPrimitive, shortPrimitive);
#ifdef ASSERT
  assert(intPrimitive == shortPrimitive);
#endif
  if (shortPrimitive < 0) exit(1);
  exit(0);
}
  

/*
The above code, when compiled and run on certain systems, returns the following output:

(Result) 
Int MAXINT: 2147483647
Short MAXINT: -1
This problem may be exploitable when the truncated value is used as an array
index, which can happen implicitly when 64-bit values are used as indexes, as
they are truncated to 32 bits.

*/
