int main() {
  int *x;
  x = new int;
  delete x; /* OK */
  
  x = new int;
  delete[] x; /* BAD */
  return 0;
}
