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
