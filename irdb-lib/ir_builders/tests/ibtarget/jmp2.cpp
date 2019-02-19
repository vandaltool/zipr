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

#include <stdlib.h>
#include <stdio.h>

int add(int a, int b)
{
	int x = a + b;
	printf("add(): %d + %d = %d\n", a, b, x);
	return x;
}

int main(int argc, char **argv)
{
	int i = atoi(argv[1]);

	switch(i) 
	{
		case 20:
			printf("i = 20\n");
			break;
		case 22:
			printf("i = 22\n");
			break;
		case 23:
			printf("hello: ");
			printf("i = 23\n");
			break;
		case 24:
			printf("i = 24\n");
			break;
		case 25:
			printf("i = 25\n");
			break;
		case 26:
			printf("bar: ");
			printf("i = 26\n");
			break;
		case 27:
			printf("i = 27\n");
			break;
		case 28:
			printf("i = 28\n");
			break;
		default:
			printf("i = %d\n", i);
	}

	int val = add(i,i);
	printf("main(): %d + %d = %d\n", i, i, val);

	switch(val) 
	{
		case 20:
			val = 35;
			printf("a\n");
			break;
		case 22:
			val = add(21,31);
			printf("36\n");
			break;
		case 23:
			val = add(val, val);
			printf("37\n");
			break;
		case 24:
			val = add(val, 34);
			printf("38\n");
			break;
		case 25:
			val = 38;
			printf("39\n");
			break;
		case 26:
			val = 45;
			printf("45\n");
			break;
		case 27:
			val = 55;
			printf("55\n");
			break;
		case 28:
			val = 69;
			printf("69\n");
			break;
		case 29:
			val = 72;
			break;
		case 31:
			val = 82;
			printf("82\n");
			break;
		default:
			val = 99;
	}

	printf("val = %d\n", val);

}
