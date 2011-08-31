int int_fussy_overflow(int x, int y)
{
  printf("int_fussy_overflow: %d %d\n", x, y);
  int result = x + 10000 - y;
  return result;
}

unsigned uint_fussy_overflow(unsigned x, unsigned y)
{
  printf("uint_fussy_overflow: %u %u\n", x, y);
  unsigned result = x + 10000 - y;
  return result;
}

int signed_overflow(int x, int y)
{
  printf("signed_overflow: %d %d\n", x, y);
  int sum = x + y;  
  return sum;
}

unsigned unsigned_overflow(unsigned x, unsigned y)
{
  printf("unsigned_overflow: %u %u\n", x, y);
  unsigned sum = x + y;  
  return sum;
}

char* integer_overflow_into_malloc_1(unsigned numElements)
{
  printf("integer_overflow_into_malloc_1: %u\n", numElements);
  unsigned int size = numElements * 4; // compiler may use shifting here
  char *buf = malloc(size);
  return buf;
}

char* integer_overflow_into_malloc_2(unsigned numElements, unsigned sizePerElement)
{
  printf("integer_overflow_into_malloc_2: %u %u\n", numElements, sizePerElement);
  unsigned int size = numElements * sizePerElement; 
  char *buf = malloc(size);
  return buf;
}

char* integer_underflow(unsigned len, char *src)
{
  printf("integer_underflow: %d\n", len);
  unsigned int size;
  size = len - 2; // len = 0, size = -2
  char *comm = (char*) malloc(size + 1); // -1 (MAX_UNSIGNED_INT) passed to malloc
  memcpy(comm, src, size);
  return comm;
}

char* signed_error(int size)
{
  printf("signed_error: %d\n", size);
  return malloc(size);
}

int signed_error_bypass_check(unsigned value)
{
  printf("signed_error_bypass_check: %u\n", value);
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
  printf("trunc_error: %u %d\n", size, numElements);
  short len = size;
  return malloc(len * numElements);
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
    case 0:
      bufptr = integer_overflow_into_malloc_2(4, 4);
    break;
    case 1:
      bufptr = integer_overflow_into_malloc_2(2000000000, 4);
    break;
    case 2:
      bufptr = integer_overflow_into_malloc_1(4);
    break;
    case 3:
      bufptr = integer_overflow_into_malloc_1(4000000000);
    break;
    case 4:
      bufptr = integer_underflow(10, buf);
    break;
    case 5:
      result = signed_error_bypass_check(2048);
    break;
    case 6:
      result = signed_error_bypass_check(4000000000);
    break;
  }
}
