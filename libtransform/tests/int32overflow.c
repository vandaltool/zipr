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

  int m3 = 0x0FFFFFFF;
  int m4 = 0x0FFFFFFF;
  m3 = m3 * m4;
  printf("Value of int (mul): %d\n", m3);
}
