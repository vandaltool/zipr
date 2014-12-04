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
#include <stdlib.h>

int main(int argc, char **argv)
{
  unsigned long a = 0;
  unsigned long b = 0; 
  unsigned long d;

  if (argc >= 2)
	  a = (unsigned long) strtoul(argv[1], NULL, 10);

  if (argc >= 3)
	  b = (unsigned long) strtoul(argv[2], NULL, 10);

  d = a * b;
  printf("%lu * %lu = %lu\n", a, b, d);

  return 0;
}
