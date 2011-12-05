
int main(int argc, char **argv)
{
  unsigned a = 2000000000, b = 2000;
  unsigned x = 2000000000, y = 2000;

  unsigned e = x * y; // IMUL
  printf("%u * %u = %u\n", x, y, e);

  if (argc >= 2) 
  	a = (unsigned) atoi(argv[1]) + 1;

  if (argc >= 3)
    b = (unsigned) atoi(argv[2]) + 1;

  unsigned d = a * b; // IMUL
  printf("%u * %u = %u\n", a, b, d);

  return 0;
}
