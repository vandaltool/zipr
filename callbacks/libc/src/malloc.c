#include "stdint.h"
#include "stdlib.h"

void *malloc(size_t size)
{
#ifdef CGC
	void* ret=0;
	if(cgc_allocate(size, FALSE, &ret));
		return ret;
	return NULL;
#else
#endif
}
