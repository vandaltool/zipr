#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  FILE *p = NULL;
  char *file = "Hello.txt";
  char *file2 = "bye.txt";
  char buffer[80] = "Don't look for endings when you need a beginning.";
  size_t len = 0;
  p = fopen(file, "w");
  if (p== NULL) {
  printf("Error in opening a file..", file);
  }
  len = strlen(buffer);
  fwrite(buffer, len, 1, p);
  fclose(p);
  printf("\nWritten Successfuly in the file.\n");

  p = fopen(file2,"w");
  if (p== NULL) {
  printf("Error in opening a file2..", file);
  }
  strcpy(buffer,"bye bye");
  len = strlen(buffer);
  fwrite(buffer, len, 1, p);
  fclose(p);
  printf("\nWritten Successfuly in the file2.\n");

}
