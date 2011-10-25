/*
Unexpected Sign Extension

Description Summary
The software performs an operation on a number that causes it to be sign extended 
when it is transformed into a larger data type. When the original number is 
negative, this can produce unexpected values that lead to resultant weaknesses.


Common Consequences
Scope  
Integrity
Confidentiality
Availability 

Effect
When an unexpected sign extension occurs in code that operates directly on memory 
buffers, such as a size value or a memory index, then it could cause the program 
to write or read outside the boundaries of the intended buffer. If the numeric 
value is associated with an application-level resource, such as a quantity or 
price for a product in an e-commerce site, then the sign extension could produce 
a value that is much higher (or lower) than the application's allowable range.
 

Example 1

The following code reads a maximum size and performs a sanity check on that size. 
It then performs a strncpy, assuming it will not exceed the boundaries of the 
array. While the use of "short s" is forced in this particular example, short 
int's are frequently used within real-world code, such as code that processes 
structured data.
(Bad Code)Example Language: C

 - Crashes on strncpy(?,?,-1)
@BAD_ARGS xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
@ATTACK_SUCCEEDED_CODE 139

 */ 

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#ifdef ASSERT
   #include <assert.h>
#endif


int GetUntrustedInt () {
  return(0x0000FFFF);
}


void main (int argc, char **argv) {
  char path[256];
  char *input;
  int i;
  short s;
  unsigned int sz;


  i = GetUntrustedInt();
  s = i;
  /* s is -1 so it passes the safety check - CWE-697 */
// BJM no assert we don't want to catch this or we 
// will never get to the CVE we are looking at
  if (s > 256) {
    exit(3);
  }

  /* s is sign-extended and saved in sz */
  sz = s;
//BJM CWE 194 sign-extended

  /* output: i=65535, s=-1, sz=4294967295 - your mileage may vary */
  printf("i=%d, s=%d, sz=%u\n", i, s, sz);

  // input = GetUserInput("Enter pathname:");
  input = argv[1];

  /* strncpy interprets s as unsigned int, so it's treated as MAX_INT
  (CWE-195), enabling buffer overflow (CWE-119) */
#ifdef ASSERT
//printf("s = %i sizeof input = %i\n",s,sizeof(input));  
assert(s>0);
assert(s <= sizeof(path));
#endif
  strncpy(path, input, s);
  path[255] = '\0'; /* don't want CWE-170 */
  printf("Path is: %s\n", path);
  exit(0);
}

