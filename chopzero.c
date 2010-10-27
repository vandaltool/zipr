#include <stdio.h>

#define MAXLINE 1000

int main() {
   char linebuf[MAXLINE];
   int addr, size;

   while (!feof(stdin)) {
      fgets(linebuf, MAXLINE-1, stdin);
      sscanf(linebuf, "%x %d", &addr, &size);
      if ((size != 0) && (addr != 0)) {
         fputs(linebuf, stdout);
      }
   }

   return 0;
}

