/*
there's another classic error in many implementations of itoa:
"applied to n equal to INT_MIN (0x80000000), the negation -n will evaluate to
INT_MIN instead of INT_MAX+1." Again, please give us an example with the buggy
itoa; in this case it can just call it on an integer passed on the command
line. (This example may have a lower priority; I'm not clear what CWE class it
belongs to, but "The case of itoa is compelling: the ?rst edition of The C
Programming Language in 1978 [34] contained the integer ove?ow problem just
mentioned; the problem was noted in the second edition in 1988 (and its
solution left in exercise), but many currently available implementations, such
as the one from project itoa on sourceforge.net, still suffer from the same
problem.")

@BAD_ARGS -2147483648 
@ATTACK_SUCCEEDED_OUTPUT_NOT_CONTAINS -2147483648
   bjm for this string uncoment the  printf("n should be + = %i\n",n);
   ATTACK_SUCCEEDED_OUTPUT_CONTAINS n should be + = -2147483648 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void my_reverse(char s[]) {
    int c, i, j;
    for ( i = 0, j = strlen(s)-1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void my_itoa(int n, char* buf)
{
char *save =buf;
   // Handle negative
   if (n < 0)
   {
       *buf++ = '-';
    //   printf("n = %i\n",n);
       n = -n;
    //   printf("n should be + = %i\n",n);
       // Output digits
       do{
          *buf++ = (n % 10) + '0';
       }while (n /= 10);
       *buf = '\0'; 
       *save++;
       my_reverse(save);
   }else {
     // Output digits
     do{
       *buf++ = (n % 10) + '0';
     }while (n /= 10);
   *buf = '\0'; 
   my_reverse(save);
   }
}
int main(int argc, char **argv) 
{
  int n1 = atoi(argv[1]);
  char num[20];
  my_itoa(n1, num);
  printf("string = %s\n",num);
  exit(0);
}
