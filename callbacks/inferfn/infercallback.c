/*
 * Copyright (c) 2014 - Zephyr Software LLC
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

#include "infer.h"

#ifdef DEBUG
	#define print_str_debug print_str
	#define print_int_debug print_int
	#define assert(a) {if(!a) print_str("assert failure!"), cgc_terminate(254);}
#else
	#define print_str_debug(a) 
	#define print_int_debug(a) 
	#define assert(a)
#endif


#ifdef DEBUG
#if 0 // figure out a way to link these funcs iff someone needs them?
#define write_fd 2
void print_str(char *s)
{
	write(write_fd,s,strlen(s));
}
void print_int(int x)
{
	char buf[100];
	itox(x,buf);
	write(write_fd,buf,strlen(buf));
}
#endif
#endif



typedef struct 
{
        int edi;
        int esi;
        int ebp;
        int esp_dummy;
        int ebx;
        int edx;
        int ecx;
        int eax;
        int flags;
} reg_values_t;


void test_handler(volatile int p_retaddress, volatile int p_address, volatile int p_exitPolicy, reg_values_t rv)
{
#ifdef DEBUG
	print_str("Detected buffer overflow at ");
	print_int(p_address);
	print_str("\n");
#endif

	commandLoop();

	cgc_terminate(3);
}

