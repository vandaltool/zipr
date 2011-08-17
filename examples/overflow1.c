int test_mul32_overflow(int value)
{
  printf("tests_mul32_overflow(%d): enter\n", value);
  int result = value * 2000000; 
  printf("tests_mul32_overflow -- about to exit\n");
  return result;
}

int test_addsub_overflow(int value)
{
  printf("tests_addsub_overflow(%d): enter\n", value);
  int result = value + 2000000000; 
  printf("tests_addsub_overflow -- about to exit\n");
  return result;
}

char *test_sign_unsign(int size)
{
  return malloc(size * 1024); 
}

int main(int argc, char **argv)
{
  printf("\n\n----------------------------");

  printf("main(): enter\n");

  if (argc != 2)
  {
    printf("usage: %s <number>\n", argv[0]);
    return 1;
  }

  int value = atoi(argv[1]);

  printf("addsub: value_overflow=%d\n", test_addsub_overflow(value));
  printf("mul: value_overflow=%d\n", test_mul32_overflow(value));


  char *x = test_sign_unsign(value);
  sprintf(x,"x");
  printf("x=%s\n", x);

  printf("----------------------------\n\n");
  return 0;
}
