#include <stdio.h>

unsigned S = 1234;

int main(int argc, char **argv)
{
  unsigned short x;
  unsigned short y = 15;

  x = 0xFFFF;
  x++;

  printf("Value of short int (add): %u\n", x);

  y = x + y;
  printf("Value of short int (add): %u\n", y);

  S = S + y;
  printf("Value of short int (add): %u\n", S);

  y = S + x;
  printf("Value of short int (add): %u\n", y);
}
