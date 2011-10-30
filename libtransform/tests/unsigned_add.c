int main(int argc, char **argv)
{
  unsigned delta = 0xFFFFFFF0;
  unsigned base = 0xFFFFFFF0;
  unsigned result = delta;
  
  if (delta > 0)
  	delta++;

  result = base + delta;

  printf("%u + %u = %u\n", base, delta, result);
}
