/*
Signed to Unsigned Conversion Error

Description Summary
A signed-to-unsigned conversion error takes place when a signed primitive is used 
as an unsigned value, usually as a size variable. 

Extended Description
It is dangerous to rely on implicit casts between signed and unsigned numbers because 
the result can take on an unexpected value and violate assumptions made by the program. 

Scope Effect 
Availability Conversion between signed and unsigned values can lead to a variety of 
errors, but from a security standpoint is most commonly associated with integer 
overflow and buffer overflow vulnerabilities.


Example 1
In this example the variable amount can hold a negative value when it is returned. 
Because the function is declared to return an unsigned int, amount will be implicitly 
converted to unsigned.
(Bad Code)Example Language: C 

@BAD_ARGS -1
@GOOD_ARGS 1
@ATTACK_SUCCEEDED_CODE 1

*/
#include <stdlib.h>

unsigned int readdata (int n) {
int amount = n;
return amount;
}


int main(int argc, char **argv)
{
  if (argc < 2) exit(2);
  int n = atoi(argv[1]);
  if (readdata(n) > 0 && n < 0) exit(1);
  exit(0);
}

/*
If the error condition in the code above is met, then the return value of readdata() will be 4,294,967,295 on a system that uses 32-bit integers. 
*/
 


