#include <cstdlib>

int main() {
  int *x;
  x = (int *) malloc(sizeof *x);
  free(x); /* OK */
  
  x = (int*) malloc(sizeof *x);
  delete x; /* BAD */

  return 0;
}
