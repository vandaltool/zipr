#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>

enum {BUFSIZE = 24};

char grade = 'D';
char Name[BUFSIZE];

void readString_xxx(char *in, char *s) {
   char buf[BUFSIZE];
   int i = 0; 
   int c;

   for (;;) 
   {
      c = in[i];
      if ((c == '\0') || (c == '\n')) 
         break;

      buf[i] = c;
      i++;
   }
   buf[i] = '\0';

   for (i = 0; i < BUFSIZE; i++) 
      s[i] = buf[i];
}


int main(int argc, char * argv[])
{
   if (argc == 2)
      readString_xxx(argv[1], Name);

   if (strcmp(Name, "Wizard in Training") == 0) 
      grade = 'B';

   printf("Thank you, %s.\n", Name);
   printf("I recommend that you get a grade of %c on this assignment.\n", grade);

   return 0;
}
