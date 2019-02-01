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

int main(int argc, char **argv)
{
  unsigned a = 2000000000, b = 2000;
  unsigned x = 2000000000, y = 2000;

  unsigned e = x * y; // IMUL
  printf("%u * %u = %u\n", x, y, e);

  if (argc >= 2) 
  	a = (unsigned) atoi(argv[1]) + 1;

  if (argc >= 3)
    b = (unsigned) atoi(argv[2]) + 1;

  unsigned d = a * b; // IMUL
  printf("%u * %u = %u\n", a, b, d);

  return 0;
}
