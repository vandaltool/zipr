/*This code came from UVA it has both a integer wrap in the malloc 
The scanf may index out of bounds?
It also has a resource drain due to the malloc?

This routine is doing a malloc of a negative number, but
this gets typecasted to an unsigned, which gives you 
a very large number.  On some machines, this will still 
succeed if there is enough memory.  - DAH

@GOOD_ARGS 4 <Example_UVA_good.txt
@BAD_ARGS -4 <Example_UVA_good.txt
    @ATTACK_SUCCEEDED_CODE 139
@ATTACK_SUCCEEDED_OUTPUT_CONTAINS malloc = 4294967280
  */
  
#include <stdlib.h>
#include <stdio.h>
#ifdef ASSERT
  #include <assert.h>
#endif

int main(int argc, char *argv[]) {
    int cnt, idx, *elems;

    cnt = atoi(argv[1]);
printf(" malloc = %u\n",sizeof(int) * cnt);
    elems = malloc(sizeof(int) * cnt);

#ifdef ASSERT
assert( (sizeof(int) * cnt)>0 );
#endif

   for(idx = 0; idx < cnt; ++idx) {
        scanf("%d\n", elems + idx);

#ifdef ASSERT
printf(" last address of mem alloced %x \n",&elems[cnt-1]);
printf(" address just written to %x \n",(elems + idx));
assert(idx < (cnt-1));
#endif

    }

    return 0;
}

