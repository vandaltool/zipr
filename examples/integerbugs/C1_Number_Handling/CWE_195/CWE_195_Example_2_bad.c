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

Example 2
In this example, depending on the return value of accecssmainframe(), the variable 
amount can hold a negative value when it is returned. Because the function is declared 
to return an unsigned value, amount will be implicitly cast to an unsigned number.
(Bad Code)Example Language: C 

@BAD_ARGS
@ATTACK_SUCCEEDED_CODE 1
*/

#include <stdlib.h>

int accessmainframe()
{
  return -1;
}

unsigned int readdata () {
int amount = 0;
amount = accessmainframe();
return amount;
}


int main(int argc, char **argv)
{
  unsigned int n = readdata();
  if (n > 1000) exit(1);
  exit(0);
}

/*
If the return value of accessmainframe() is -1, then the return value of readdata() 
will be 4,294,967,295 on a system that uses 32-bit integers.
*/


