/*
Integer Overflow or Wraparound  


Description Summary
The software performs a calculation that can produce an integer overflow or wraparound, 
when the logic assumes that the resulting value will always be larger than the original 
value. This can introduce other weaknesses when the calculation is used for resource 
management or execution control. 

Extended Description
An integer overflow or wraparound occurs when an integer value is incremented to a 
value that is too large to store in the associated representation. When this occurs, 
the value may wrap to become a very small or negative number. While this may be 
intended behavior in circumstances that rely on wrapping, it can have security 
consequences if the wrap is unexpected. This is especially the case if the integer 
overflow can be triggered using user-supplied inputs. This becomes security-critical 
when the result is used to control looping, make a security decision, or determine 
the offset or size in behaviors such as memory allocation, copying, concatenation, etc. 

Common Consequences
Scope 			Effect 
Availability 	Technical Impact: DoS: crash / exit / restart; DoS: resource consumption (CPU)
                Integer overflows generally lead to undefined behavior and therefore 
                crashes. In the case of overflows involving loop index variables, the 
                likelihood of infinite loops is also high.
 
Integrity 	Technical Impact: Modify memory
                If the value in question is important to data (as opposed to flow), 
                simple data corruption may occur. Also, if the integer overflow results 
                in a buffer overflow condition, data corruption may take place.
 
Access Control
Integrity 	Technical Impact: Execute unauthorized code or commands
                Integer overflows can sometimes trigger buffer overflows which can be 
                used to execute arbitrary code. This is usually outside the scope of a 
                program's implicit security policy.
 
Example 2
Integer overflows can be complicated and difficult to detect. The following example is an attempt to show how an integer overflow may lead to undefined looping behavior:
(Bad Code)Example Language: C 

@GOOD_ARGS 100 <Example2_good.txt
@BAD_ARGS 32767 <Example2_bad.txt
@ATTACK_SUCCEEDED_CODE 1
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

short int getFromInput(char* buf)
{
  scanf("%s\n", buf);
  return strlen(buf);
}

#define SOMEBIGNUM 100000
void doit(int MAXGET)
{
  short int bytesRec = 0;
  char buf[SOMEBIGNUM];
  int count = 0;

  while(bytesRec < MAXGET) {
    bytesRec += getFromInput(buf+bytesRec);
    if (count++ > 10000) { exit(1); }
  }
}

int main(int argc, char **argv)
{
  doit(atoi(argv[1]));
  exit(0);
}

/*
In the above case, it is entirely possible that bytesRec may overflow, continuously creating a lower number than MAXGET and also overwriting the first MAXGET-1 bytes of buf.

*/   
