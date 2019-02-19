int main() {
  int *x;
  x = new int[1];
  delete[] x; /* OK */
  
  x = new int[1];
  delete x; /* BAD */
  
  return 0;
}
