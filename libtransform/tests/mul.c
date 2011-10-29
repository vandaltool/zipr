int main(int argc, char **argv)
{
  int a = atoi(argv[1]);
  int b = atoi(argv[2]);

  int c = a * b;

  printf("%d * %d = %d\n", a, b, c);

  unsigned d = (unsigned)a * (unsigned)b;
  printf("%u * %u = %u\n", (unsigned)a, (unsigned)b, d);
}
