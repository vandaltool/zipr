
int main(int argc, char **argv)
{
  unsigned a = (unsigned) atoi(argv[1]);
  unsigned b = (unsigned) atoi(argv[2]);
  unsigned d = a * b;

  printf("%u\n", d);

//  printf("%u * %u = %u\n", a, b, d);
}
