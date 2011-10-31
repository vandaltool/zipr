#include <stdio.h>

// pass in 2 arguments
main(int argc, char **argv)
{
  int x, result;
  int y;

  if (argc != 3)
  {
    fprintf(stderr,"usage: %s <arg1> <arg2>\n", argv[0]);
    exit(1);
  }

  x = strtol(argv[1], NULL, 10);
  y = strtol(argv[2], NULL, 10);
  result = x + y;
  printf ("result: %d %x\n", result, result);
}

