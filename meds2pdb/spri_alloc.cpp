/*
 * spri_alloc.c - allocation routines, see below.
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

#include <stdlib.h>
#include <string.h>
#include "spri_alloc.h"

//
// Wrap memory management routines
//
// This will make it easier to integrate back within the Strata library 
// as we could simply call strata_allocate_type and strata_deallocate_type
// instead of malloc/free
//

void* spri_allocate_type(int p_size)
{
  return malloc(p_size);
}

void* spri_deallocate_type(void *p_ptr, int p_size)
{
  free(p_ptr);
  return NULL;
}

char* spri_strdup(const char *p_ptr)
{
  return strdup(p_ptr);
}
