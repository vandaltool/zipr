/*
 * Copyright (c) 2013, 2014 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

#include <stdio.h>
#include <limits.h>

int main() {
   unsigned short us15, us15copy;
   signed short s15copy, s15inccopy, s16trunc;
   unsigned int us17;
   signed int sneg1;
   int count;

   printf("Enter 32767 to trigger errors of signedness and overflow, smaller otherwise.\n");

   printf("Enter unsigned short value: ");

   count = scanf("%hu", &us15);

   printf("\n");

 

   if (count != 1) printf("\nINPUT ERROR\n");

   if (32768 > (us15 + 1)) { /* Conditional branch here implies unsigned */

     printf("Bypassing signedness and overflow errors.\n");

   }
   else if (32767 == us15) {

     printf("Preparing to trigger signedness and overflow errors.\n");

   }
   else {

     printf("Preparing to trigger signedness error.\n");

   }

   s15copy = us15;
   printf("Input value: %hu Signed copy: %hd\n ", us15, s15copy);
   ++s15copy;  /* OVERFLOW ERROR */

   ++us15copy; /* Check for overflow, but no error */

   printf("Signed short gets unsigned value: %hd\n", us15copy);
   s15inccopy = us15copy; /* SIGNEDNESS ERROR */

   printf(" (Overflow) Signed copy then increment: %hd\n", s15copy);

   printf(" (Signedness) Increment then signed copy: %hd\n", s15inccopy);

   printf("\nEnter -1 to trigger underflow, positive int otherwise: ");

   count = scanf("%d", &sneg1);

   if (count != 1) printf("\nINPUT ERROR\n");

   printf("\n");

   if (-1 < sneg1) { /* Conditional branch here implies signed */

     printf("Bypassing underflow error.\n");

   }

   else if (-1 == sneg1) {

     printf("Preparing to trigger underflow error.\n");

   }

   sneg1 -= INT_MAX; /* underflow check should occur, but no error. */


   printf("Value minus INT_MAX = %d\n", sneg1);

   --sneg1; /* UNDERFLOW ERROR */

   printf(" (Underflow) Value minus INT_MAX minus 1 = %d\n", sneg1);

   printf("\nEnter 131071 to trigger truncation, small positive int otherwise: ");

   count = scanf("%u", &us17);

   if (count != 1) printf("\nINPUT ERROR\n");

   printf("\n");

   if (65536  > us17) { /* Conditional branch here implies unsigned */

     printf("Bypassing truncation error.\n");

   }
   else if (131071 == us17) {

     printf("Preparing to trigger truncation error.\n");

   }

   s16trunc = us17; /* TRUNCATION ERROR */

   printf("Unsigned value = %u\n", us17);

   printf(" (Truncation) Value copied to signed short = %hd\n", s16trunc);


   return 0;
}

