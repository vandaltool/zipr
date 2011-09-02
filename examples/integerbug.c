#include <stdio.h>

#define INT_MAX 2147483647
#define UINT_MAX 4294967295

int int_fussy_overflow(int x, int y)
{
  printf("int_fussy_overflow(): %d %d\n", x, y);
  int result = x + 10000 - y;
  printf("int_fussy_overflow(): result: %d\n", result);
  return result;
}

unsigned uint_fussy_overflow(unsigned x, unsigned y)
{
  printf("uint_fussy_overflow(): %u %u\n", x, y);
  unsigned result = x + 10000 - y;
  printf("int_fussy_overflow(): result: %u\n", result);
  return result;
}

int signed_overflow(int x, int y)
{
  printf("signed_overflow(): %d %d\n", x, y);
  int sum = x + y;  
  printf("signed_overflow(): sum = %d\n", sum);
  return sum;
}

unsigned unsigned_overflow(unsigned x, unsigned y)
{
  printf("unsigned_overflow(): %u %u\n", x, y);
  unsigned sum = x + y;  
  return sum;
}

char* integer_overflow_into_malloc_1(unsigned numElements)
{
  printf("integer_overflow_into_malloc_1(): %u\n", numElements);
  unsigned int size = numElements * 4; // compiler may use shifting here
  char *buf = malloc(size);
  return buf;
}

char* integer_overflow_into_malloc_2(unsigned numElements, unsigned sizePerElement)
{
  printf("integer_overflow_into_malloc_2(): %u %u\n", numElements, sizePerElement);
  unsigned int size = numElements * sizePerElement; 
  char *buf = malloc(size);
  return buf;
}

char* integer_underflow(unsigned len, char *src)
{
  printf("integer_underflow(): %d\n", len);
  unsigned int size;
  size = len - 2; // len = 0, size = -2
  char *comm = (char*) malloc(size + 1); // -1 (MAX_UNSIGNED_INT) passed to malloc
  memcpy(comm, src, size);
  return comm;
}

#define BUFF_SIZE 10

// if len is < 0, this will bypass the check 
// which will result in an overflow of buf[] 
char* sign_error_buffer_overflow(int len, char *src)
{
  printf("sign_error_buffer_overflow(): %d\n", len);
  char buf[BUFF_SIZE];
  if (len < BUFF_SIZE) {
    printf("Copying %u bytes into buffer of size %u\n", len, BUFF_SIZE);
    return memcpy(buf, src, len);
  }
  else {
    return NULL;
  }
}

char* signed_error(int size)
{
  printf("signed_error(): %d\n", size);
  return malloc(size);
}

int signed_error_bypass_check(unsigned value)
{
  printf("signed_error_bypass_check(): %u\n", value);
  int x = value;
  if ( x > 1024 ) 
  {
    printf("too big\n");
    return 1;
  }
  else
  {
    printf("passed upper bound check\n");
    return 0;
  }
}

char* trunc_error(unsigned size, int numElements)
{
  printf("trunc_error(): %u %d\n", size, numElements);
  short len = size;
  return malloc(len * numElements);
}

short sign_extend_char_short(char c)
{
  short s;
  return s = c;
}

short sign_extend_char_long(char c)
{
  long l;
  return l = c;
}

short sign_extend_short_long(short s)
{
  long l;
  return l = s;
}

int main(int argc, char **argv)
{
  int selector = 0;
  int myint;
  int result;
  char *bufptr;
  char buf[16] = "hello";

  if (argc == 2)
    selector = atoi(argv[1]);

  switch(selector)
  {
    // good inputs here
    case 0:
      int_fussy_overflow(10,12);
      uint_fussy_overflow(10,12);
      bufptr = integer_overflow_into_malloc_2(1, 4);
      bufptr = integer_overflow_into_malloc_1(1);
      bufptr = integer_underflow(10, buf);
      result = signed_error_bypass_check(10);
      bufptr = trunc_error(10, 10);
      signed_overflow(2, 3);
      break;

    // "bad" inputs here
    case 1:
      bufptr = integer_overflow_into_malloc_2(2000000000, 4);
      break;
    case 2:
      bufptr = integer_overflow_into_malloc_1(4000000000);
      break;
    case 3:
      bufptr = integer_underflow(1, buf);
      break;
    case 4:
      result = signed_error_bypass_check(4000000000);
      break;
    case 5:
      bufptr = trunc_error(65000, 10);
      break;
    case 6:
      int_fussy_overflow(INT_MAX,INT_MAX);
      break;
    case 7:
      signed_overflow(INT_MAX, INT_MAX);
      break;
    case 8:
      signed_overflow(INT_MAX, INT_MAX);
      break;
    case 9:
      sign_error_buffer_overflow(-1, buf);
      break;
  }
}
