/*
 * Copyright (c) 2014-2015 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */

#include <stdio.h>
#include <stdlib.h>

int add(int a, int b)
{
	printf("%d + %d = %d\n", a, b, a+b);
	return a + b;
}

int sub(int a, int b)
{
	printf("%d - %d = %d\n", a, b, a-b);
	return a - b;
}

int mul(int a, int b)
{
	printf("%d * %d = %d\n", a, b, a*b);
	return a * b;
}

int main(int argc, char**argv)
{
	int (*fn)(int,int);

	if (argc % 2)
		fn = &add;
	else
		fn = &sub;

	int result = (*fn)(argc, argc+1);
	printf("result = %d\n", result);
}
