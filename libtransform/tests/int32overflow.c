#include <stdio.h>

int main(int argc, char **argv)
{
  unsigned int x;

  x = 0xFFFFFFFF;
  x++;

  printf("Value of unsigned int (add): %u\n", x);

  unsigned int s;
  s = 0;
  s--;
  printf("Value of unsigned int (sub): %u\n", s);

  unsigned int m1 = 5;
  unsigned int m2 = 0xFFFFFFFF;
  m1 = m1 * m2;
  printf("Value of unsigned int (mul): %u\n", m1);

}
