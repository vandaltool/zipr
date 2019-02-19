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
      c = fgetc(f);
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
   f = fopen("data.txt", "r");

   readString(Name);

   if (strcmp(Name, "Wizard in Training") == 0) 
      grade = 'B';

   printf("Thank you, %s.\n", Name);
   printf("I recommend that you get a grade of %c on this assignment.\n", grade);

//   exit(0);
}

