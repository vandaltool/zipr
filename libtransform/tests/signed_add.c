int main(int argc, char **argv)
{
  int a = atoi(argv[1]);
  int b = atoi(argv[2]);
  volatile int result;
  
  printf("a = %d; b = %d\n", a, b);

  result = 1 + a + b;

  printf("1 + a=%d + b=%d = %d\n", a, b, result);
}
