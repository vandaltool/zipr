#include "stdint.h"
#include "stdlib.h"

void *malloc(size_t size)
{
	void* ret=0;
	if(cgc_allocate(size, FALSE, &ret));
		return ret;
	return NULL;
}
