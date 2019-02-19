/*
 * spri_alloc.h - allocation routines, see below.
 *
 * Copyright (c) 2011 - Zephyr Software
 *
 * This file is part of the Strata dynamic code modification infrastructure.
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

#ifndef _SPRI_ALLOC_
#define _SPRI_ALLOC_

/*
 *  Memory allocation/deallocation wrapper
 */

void* spri_allocate_type(int);
void* spri_deallocate_type(void *, int);
char* spri_strdup(const char *);

#endif
