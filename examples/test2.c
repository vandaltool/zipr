void foo()
{
  char buf1[8] = "hello";
  char buf2[8];
  int i;

  for (i = 0; i < 8; ++i)
  {
    buf2[i] = buf1[i];
  }

  printf("%s %s\n", buf1, buf2);
}

main()
{
  foo();
}
