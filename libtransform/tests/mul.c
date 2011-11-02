
int main(int argc, char **argv)
{
  unsigned a = (unsigned) atoi(argv[1]);
  unsigned b = (unsigned) atoi(argv[2]);
  unsigned d = a * b; // IMUL

  printf("%u\n", d);
}
