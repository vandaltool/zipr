/*
"in the case of function binary_search applied to low and high both equal to
(0x40000000), the computation low + high will evaluate to INT_MIN instead of
(INT_MAX+1)/2." I'd like a test that reads the contents of a file into an array
and calls a faulty binary_search on the array.

BJM
This code has more problems then the integer wrap.  Val is going to get
set to some random data in memory depending on what values are passed in.

@BAD_ARGS   <bad.dat
@ATTACK_SUCCEEDED_OUTPUT_CONTAINS -1073741824
*/
#include <stdio.h>
#include <stdlib.h>

int binary_search(int* arr, int low, int high, int key)
{
   printf("low = %i high = %i mid  = %i \n",low,high,( (low + high) / 2));
   while (low <= high)
   {
       // Find middle value
       int mid = (low + high) / 2;
       int val = arr[mid];
       printf("value %i \n",val);
       low++; 
      // Refine range
   }
}
int main(void) 
{
  int n1 = 0;
  int n2 = 0;
int x = 0x40000000;
 fscanf(stdin,"%x",&n1);
 fscanf(stdin,"%x",&n2);

printf("%d %d %d\n", n1, n2, x);

  FILE *f ;
  if(f= fopen("data.txt", "rb")){

     fseek(f, 0, SEEK_END);  
     long nbytes = ftell(f);
     fseek(f, 0, SEEK_SET); 

     printf("File size = %ld malloc\n",nbytes); 
     int *bytes = malloc(nbytes+1); 
     fread(bytes, 1,nbytes, f); 
     fclose(f);  

     int i;
     printf("File buffer dump\n"); 
     for(i = 0;i < nbytes;++i)
        printf("%c", ((char *)bytes)[i]);

//     binary_search(bytes, 0x40000000, 0x40000000, 32);
     binary_search(bytes, n1, n2, 32);

     free(bytes); // free allocated memory 
    }else{
       printf("file open failed \n");
    }
}

