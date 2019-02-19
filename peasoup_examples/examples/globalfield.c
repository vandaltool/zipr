#include <stdio.h>
#include <stdlib.h>

#define TRUE 1
#define FALSE 0


struct ASI {
  int valid;
  float (*pt2func)(float, float);
  int buf[128];
  int *uid_ptr;
};  

float FloatingAdd(float a, float b) {
  return (a + b);
}

float FloatingSub(float a, float b) {
  return (a - b);
}

struct ASI GlobalStruct1;

int main() {
  int x, index;
  float y, z, result;

  GlobalStruct1.valid = FALSE;
  GlobalStruct1.uid_ptr = 0;
   y = 2.0;
   z = 13.0;

   printf("Enter a positive integer. Greater than 128 overflows.\n");
   scanf("%d", &x);

   GlobalStruct1.uid_ptr = &x;
   if (x < 0) {
     GlobalStruct1.pt2func = &FloatingAdd;
   }
   else {
     GlobalStruct1.pt2func = &FloatingSub;
   }

   result = GlobalStruct1.pt2func(y, z);

   for (index = 0; index < x; ++index) { 
     GlobalStruct1.buf[index] = index;  // buffer overflow vulnerability
   }
     
   GlobalStruct1.valid = TRUE;
   printf("x = %d y = %f z = %f result = %f uid_ptr=%xl\n", x, y, z, result, GlobalStruct1.uid_ptr);
   return 0;
}
