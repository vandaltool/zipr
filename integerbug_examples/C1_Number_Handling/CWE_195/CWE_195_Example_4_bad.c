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

Example 4
This example processes user input comprised of a series of variable-length structures. 
The first 2 bytes of input dictate the size of the structure to be processed.
(Bad Code)Example Language: C 

@BAD_ARGS <example4_bad.dat
@GOOD_ARGS <example4_good.dat
@ATTACK_SUCCEEDED_CODE 139

*/

#include <stdlib.h>
#include <memory.h>
#include <stdio.h>

void process(char* buf) {}

char* processNext(char* strm) {
  char buf[512];
  short len = *(short*) strm;
  strm += sizeof(len);
  if (len <= 512) {
    memcpy(buf, strm, len);
    process(buf);
    return strm + len;
  }
  else {
    return 0;
  }
}

int main(int argc, char **argv)
{
  char buf[512];
  int n = 0;
  while ((buf[n++] = getchar()) != EOF) {}
printf("A\n");

  processNext(buf);
printf("A\n");
  exit(0);
}


/*
The programmer has set an upper bound on the structure size: if it is larger
than 512, the input will not be processed. The problem is that len is a signed
short, so the check against the maximum structure length is done with signed values,
but len is converted to an unsigned integer for the call to memcpy() and the negative 
bit will be extended to result in a huge value for the unsigned integer. 
If len is negative, then it will appear that the structure has an appropriate size
(the if branch will be taken), but the amount of memory copied by memcpy() will 
be quite large, and the attacker will be able to overflow the stack with data
in strm. 

*/

