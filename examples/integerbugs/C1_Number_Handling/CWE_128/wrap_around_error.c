/*
@GOOD_ARGS 123
@BAD_ARGS 32767
@ATTACK_SUCCEEDED_CODE 1

Wrap-around Error

Description Summary
Wrap around errors occur whenever a value is incremented past the maximum value for its 
type and therefore "wraps around" to a very small, negative, or undefined value. 

Common Consequences
Scope 			Effect 
Availability 	Wrap-around errors generally lead to undefined behavior, infinite loops, 
                and therefore crashes.

Integrity 	If the value in question is important to data (as opposed to flow), simple 
                data corruption has occurred. Also, if the wrap around results in other conditions 
                such as buffer overflows, further memory corruption may occur.
 
Integrity 	A wrap around can sometimes trigger buffer overflows which can be used to 
                execute arbitrary code. This is usually outside the scope of a program's 
                implicit security policy.

Background Details
Due to how addition is performed by computers, if a primitive is incremented past the 
maximum value possible for its storage space, the system will not recognize this, and 
therefore increment each bit as if it still had extra space. Because of how negative 
numbers are represented in binary, primitives interpreted as signed may "wrap" to 
very large negative values.

 
*/
 
#include <stdio.h>
#include <stdlib.h>
#ifdef ASSERT
#include <assert.h>
#include <limits.h>
#endif

int main(int argc, char**argv)
{
  short int i = atoi(argv[1]);

  i++;
  printf("%hi\n", i);
  printf("%i\n", atoi(argv[1]));
#ifdef ASSERT
/*bjm
short signed: -32768 to 32767
short unsigned: 0 to 65535
long signed:-2147483648 to 2147483647 (Default unless you're using DOS)
long unsigned: 0 to 4294967295
*/
assert( (atoi(argv[1])<SHRT_MAX) && (atoi(argv[1])>SHRT_MIN-2));
#endif
/*bjm 
This may be training grace.  It is also slightly wrong the assert 
checks the right range of values acounting for i++   
*/
if (i < 0) { exit(1); }
  exit(0);
}

