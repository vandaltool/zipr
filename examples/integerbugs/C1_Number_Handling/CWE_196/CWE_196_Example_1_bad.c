/*
   Unsigned to Signed Conversion Error

Description Summary
An unsigned-to-signed conversion error takes place when a large unsigned
primitive is used as a signed value. 

Common Consequences
Scope 			Effect 
Availability 		Incorrect sign conversions generally lead to undefined behavior, and 
                        therefore crashes.
 
Integrity 		If a poor cast lead to a buffer overflow or similar condition, data integrity may be affected.
 
Integrity 		Improper signed-to-unsigned conversions without proper checking can 
                        sometimes trigger buffer overflows which can be used to execute 
                        arbitrary code. This is usually outside the scope of a program's implicit 
                        security policy.
 
Example 1
In the following example, it is possible to request that memcpy move a much
larger segment of memory than assumed:
(Bad Code)Example Language: C 

@GOOD_ARGS 1
@BAD_ARGS -1
@ATTACK_SUCCEEDED_CODE 139

*/

#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#ifdef ASSERT
  #include <assert.h>
#endif


int returnChunkSize(void * buf, int n) {
/* if chunk info is valid, return the size of usable memory,
* else, return -1 to indicate an error
*/
  return n;
}

int main(int argc, char **argv) {
  char destBuf[512];
  char* srcBuf = "Hello World";
  if (argc < 2) exit(2);
  int n = atoi(argv[1]);
  memcpy(destBuf, srcBuf, (returnChunkSize(destBuf, n)-1));
#ifdef ASSERT
  assert(isdigit(argv[1][0]));  
  assert( (sizeof(destBuf) > (returnChunkSize(destBuf, n)-1)) && ((returnChunkSize(destBuf, n)-1) > -1) );
#endif
  exit(0);
}

/*
If returnChunkSize() happens to encounter an error, and returns -1,
memcpy will assume that the value is unsigned and therefore interpret it as
MAXINT-1, therefore copying far more memory than is likely available in the
destination buffer.
*/
