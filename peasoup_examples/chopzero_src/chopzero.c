#include <stdio.h>
#include <assert.h>

#define MAXLINE 1000

int main() {
   char linebuf[MAXLINE];
   int addr, size;

   while (!feof(stdin)) {
      int res;
      char* resp;
      resp=fgets(linebuf, MAXLINE-1, stdin);
      assert(resp==NULL || resp==linebuf);
      res=sscanf(linebuf, "%x %d", &addr, &size);
      assert(res>=0);
      if ((size != 0) && (addr != 0)) {
         fputs(linebuf, stdout);
      }
   }

   return 0;
}

