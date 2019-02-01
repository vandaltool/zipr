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

// from smartfuzz paper
// ./simpletest.exe -2147483659 will trigger the Surprise
int main (int argc, char** argv)
{
	int i = atol(argv[1]); //[i is signed]
	unsigned int j = 0;

	if (i < 10) //[i is signed]
	{
		j = i;   // catch here? [unsigned = signed]
		if ( j > 50) //[j is unsigned]
		{
		 printf("Surprise! \n"); 
		 return 1;
		}
	}

return 0;

}
