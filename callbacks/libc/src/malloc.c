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

#include "stdint.h"
#include "stdlib.h"

void *malloc(size_t size)
{
#ifdef CGC
	void* ret=0;
	if(cgc_allocate(size+sizeof(int), FALSE, &ret) == 0)
	{
		*(size_t*)ret=size;
		return ret;
	}
	return NULL;
#else
	assert(0);
#endif
}

void free(void* ptr)
{
	ptr=ptr-sizeof(size_t);
	cgc_deallocate(ptr,*(size_t*)ptr);
#ifdef CGC
#else
	assert(0);
#endif
}
