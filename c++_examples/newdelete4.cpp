#include <cstdlib>

int main() {
  int *x;
  x = new int;
  delete x; /* OK */
  
  x = new int;
  free(x); /* BAD */

  return 0;
}
