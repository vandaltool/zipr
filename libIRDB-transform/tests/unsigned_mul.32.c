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

#include <stdlib.h>

int main(int argc, char **argv)
{
  unsigned int a = strtoul(argv[1], NULL, 10);
  unsigned int b = strtoul(argv[2], NULL, 10);
  unsigned int d = a * b;

/*
  if (a < 5 || a < b || d > 27 + a)
    printf("force unsigned annotation\n");
*/

  printf("%u * %u = %u\n", a, b, d);

  return 0;
}
