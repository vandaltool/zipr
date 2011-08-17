#include <stdio.h>

struct point {
  double x;
  double y;
};

struct large_point {
  double x;
  char buf[128];
  double y;
};

int test_xxx_1()
{
  int  val[3];
  val[0] = 16;
  return val[0];
}

int test_xxx_2(int a, int b)
{
  int i = a +b + 6;
  int j = a * b;
  return (i + j);
}

int test_xxx_3()
{
  int i = test_xxx_2(10,11);
  return i;
}

char * test_xxx_4(char *inbuf)
{
  int i;
  char buf[1024];

  for (i = 0; i < 5; ++i) 
  {
    buf[i] = inbuf[i];
  }
  return inbuf;
}

void test_xxx_5()
{
  char buf[16] = "aaaabbbbccccddd";
  char *p = 0;
  int i;
  int j = 42;

  p = test_xxx_4(buf);
  i = test_xxx_3();
  j = i + test_xxx_2(i, j);

  printf("xxx_5: %s %d %d\n", p, i, j);
}

void test_xxx_6()
{
  char buf1[128] = "quick brown fox";
  char buf2[128] = "jumps over the lazy dog";
  char buf3[1024];

  strcpy(buf3, buf1);
  strcat(buf3, buf2);

  printf("xxx_6: %s + %s = %s\n", buf1, buf2, buf3);
  printf("xxx_6: %d %d\n", strlen(buf1) + strlen(buf2), strlen(buf3));
}

int test_xxx_7()
{
  int x[8];

  x[1] = 1;

  printf("xxx_7: %d\n", x[1]);
  return x[1];
}

void test_xxx_8(int *bogus1, int a, int *bogus2)
{
  int i;
  int x[2];

  for (i = 0; i < 2; ++i)
  {
    x[i] = -1;
  }

  printf("xxx_8: %d %d %d\n", a, x[0], *bogus2);
}


void test_xxx_9()
{
  int i;
  int x[5];

  x[0] = -2;
  for (i = 1; i < 5; ++i)
  {
    x[i] = x[i-1];
  }

  printf("xxx_9: ");
  for (i = 0; i < 5; ++i)
    printf("%d %d", i, x[i]);
  printf("%d\n", i);
}

void test_xxx_10()
{
  int buf3[2];
  int j;
  char c;

  c = 'a'; 
  j = c;     

  printf("xxx_10: %d\n", j);
}

void test_xxx_11()
{
  int a;
  char buf1[128] = "aaaaaaaaaaaaaaaaaaaaaaaaaaa";
  int b;
  char buf2[128] = "bbbbbbbbbbbbbbbbbbbbbbbbbbb";
  int c;
  char buf3[128] = "ccccccccccccccccccccccccccc";
  int cumsum = 0;

  for (a = 0; a < strlen(buf1); ++a)
  {
    if (a % 2)
      buf3[a] = buf1[a];
    else
      buf3[a] = buf2[a];

    cumsum += buf2[a] + buf1[a];
  }

  printf("xxx_11: %d %s %s %s\n", cumsum, buf1, buf2, buf3);
}

int test_xxx_12(int a, char *buf)
{
  char tmp[1024];
  if (a <= 0) return 0;
  else
  {
    int val;
    sprintf(tmp,"%d",a);
    strcat(buf,tmp);
    val = test_xxx_12(a-1, buf);
    return a + val;
  }
}

double test_xxx_13(struct point a)
{
  return a.x + a.y;
}

struct point test_xxx_14(struct point a)
{
  struct point b;
  b = a;
  b.x = 2.5;
  b.y = 3.5;
  return b;
}
struct point test_xxx_14a(struct point *a)
{
  struct point b;
  b = *a;
  b.x = 2.5;
  b.y = 3.5;
  return b;
}

struct point* test_xxx_14b(struct point *a)
{
  struct point *b;
  a->x = 2.5;
  a->y = 3.5;
  b = a;
  return b;
}

struct large_point test_xxx_15(struct large_point a)
{
  struct large_point b;
  b = a;
  b.x = 2.5;
  b.y = 3.5;
  sprintf(b.buf,"%s", "hello");
  return b;
}

int main(int argc, char **argv)
{
  int retvalue = 0;
  int bogus = 1234;
  char buf[1024] = "hello";
  retvalue = test_xxx_1();
  printf("xxx_1: %d\n", retvalue);

  retvalue = test_xxx_2(5,6);
  printf("xxx_2: %d\n", retvalue);

  retvalue = test_xxx_3();
  printf("xxx_3: %d\n", retvalue);

  printf("xxx_4: %s\n", test_xxx_4(buf));

  test_xxx_5();
  test_xxx_6();
  test_xxx_7();
  test_xxx_8(&bogus, 10, &bogus);
  test_xxx_9();
  test_xxx_10();
  test_xxx_11();

  retvalue = test_xxx_12(10, buf);
  printf("xxx_12 (recursive): %d %s\n", retvalue, buf);

  struct point pt;
  pt.x = 1.0;
  pt.y = 2.0;
  printf("xxx_13: %f\n", test_xxx_13(pt));

  pt = test_xxx_14(pt);
  printf("xxx_14: %f %f\n", pt.x, pt.y);

  struct point *pt_ptr;
  pt_ptr = test_xxx_14b(&pt);
  printf("xxx_14b: %f %f\n", pt_ptr->x, pt_ptr->y);


  pt = test_xxx_14a(&pt);
  printf("xxx_14a: %f %f\n", pt.x, pt.y);

  struct large_point lpt;
  lpt.x = 1.0;
  lpt.y = 2.0;
  strcpy(lpt.buf, "yo");
  lpt = test_xxx_15(lpt);
  printf("xxx_15: %f %f %s\n", lpt.x, lpt.y, lpt.buf);

  return 0;
}
