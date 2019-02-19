
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>

enum {BUFSIZE = 98};

char grade = 'D';
char Name[BUFSIZE];
FILE *f;

void readString(char *s) {
   char buf[BUFSIZE];
   int i = 0; 
   int c;

   for (;;) 
   {
      c = getchar();
      if ((c == EOF) || (c == '\n')) 
         break;
      buf[i] = c;
      i++;
   }
   buf[i] = '\0';

   for (i = 0; i < BUFSIZE; i++) 
      s[i] = buf[i];
}


int main(void) 
{
#if 0
/* needed for some exploit, but not really part of the program! */
   mprotect((void*)((unsigned int)Name & 0xfffff000), 1,
            PROT_READ | PROT_WRITE | PROT_EXEC);
#endif
   readString(Name);

   if (strcmp(Name, "Wizard in Training") == 0) 
      grade = 'B';

   printf("Thank you, %s.\n", Name);
   printf("I recommend that you get a grade of %c on this assignment.\n", grade);

   exit(0);
}

