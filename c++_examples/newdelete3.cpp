#include <cstdlib>

int main() {
  int *x;
  x = new int[1];
  delete[] x; /* OK */
  
  x = new int[1];
  free(x); /* BAD */

  return 0;
}
