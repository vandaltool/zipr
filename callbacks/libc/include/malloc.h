#ifndef malloc_h
#define malloc_h

#include <stdint.h>

void* malloc(size_t size);
void free(void*);
void* calloc(size_t nmemb, size_t size);

#endif
