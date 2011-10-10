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
 
*/
/*
Example 1
The following code excerpt from OpenSSH 3.3 demonstrates a classic case of integer overflow:
(Bad Code)Example Language: C 

@BAD_ARGS
@ATTACK_SUCCEEDED_CODE 139

*/

#include <stdlib.h>
#include <stdio.h>
#ifdef ASSERT
  #include <assert.h>
#endif
int packet_get_int() { return 1073741824; }
char* packet_get_string() { return "Hello World"; }


int main(int argc, char **argv)
{
  unsigned int nresp = packet_get_int();
  if (nresp > 0) {
    unsigned bytestomalloc = nresp * sizeof(char*);
    char **response = malloc(bytestomalloc);

    int i;
    for (i = 0; i < nresp; i++) response[i] = packet_get_string();
  }
#ifdef ASSERT
assert( (nresp*sizeof(char*))>0 );
#endif
  exit(0);
}

/*
If nresp has the value 1073741824 and sizeof(char*) has its typical value of 4, then the result of the operation nresp*sizeof(char*) overflows, and the argument to xmalloc() will be 0. Most malloc() implementations will happily allocate a 0-byte buffer, causing the subsequent loop iterations to overflow the heap buffer response.
*/

