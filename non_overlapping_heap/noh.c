#define _GNU_SOURCE
#include <errno.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * The approach to getting non-overlapping heaps here is pretty simple - instead
 * of simply allowing users to mmap in multiple variants and get the same
 * result, we override the glibc mmap implementation (through LD_PRELOAD) and
 * use our own instead, which allocates NUMVARIANTS multiples of the same amount
 * of memory, and assigns a different section to each variant. This allows us to
 * get most of the cases where memory is allocated, but not all of them. In some
 * cases, programs will call mmap with specific memory addresses as the targets.
 * This may indicate some sort of dependence on the memory layout, so we let the
 * program have the same memory section in all variants. In others, programs may
 * call mmap through a system call (which may even be in a non-transformed lib),
 * which would require significantly more extensive analysis to catch and handle
 * properly (and in many cases wouldn't be doable statically without solving the
 * halting problem).
 */

int nthisvar = 0;
int nnumvar  = 1;
void* initializedvariables = NULL;

extern void* mmap(void*, size_t, int, int, int, off_t);
//extern void* mmap64(void*, size_t, int, int, int, off64_t);

void* (*orig_mmap)(void*, size_t, int, int ,int, off_t) = NULL;
//void* (*orig_mmap64)(void*, size_t, int, int ,int, off64_t) = NULL;


void _init(void) {
	orig_mmap = (void*(*)(void*, size_t, int, int, int, off_t)) dlsym(RTLD_NEXT, "mmap");
	//orig_mmap64 = (void*(*)(void*, size_t, int, int, int, off64_t)) dlsym(RTLD_DEFAULT, "mmap64");
	char* sthisvar = getenv("VARIANTINDEX"); // 0-based variant index
	char* snumvar  = getenv("NUMVARIANTS");  // total number of variants running
	if(!sthisvar || !snumvar) {
		printf("didn't find environment arguments for structnoh, disabling...\n");
	} else {
		nthisvar = atoi(sthisvar);
		nnumvar  = atoi(snumvar);
		initializedvariables = (void*)1;
	}
}

void* mmap(void* address, size_t length, int protect, int flags, int filedes, off_t offset) {
	bool is_diversified = false;
	size_t originallength = length;
	void* new_mapping = MAP_FAILED;
	if(
			(address && (flags & MAP_FIXED))	// must use normal mmap, since the program is now guaranteed the destination address or failure
			|| (flags & MAP_SHARED)			// we're sharing between multiple programs, which is complex enough as it is - alignment is important, and we'd have to do funky stuff to ensure non-overlappingness with this regardless
			|| ((!flags) & MAP_ANONYMOUS)		// we're mapping a file, so the kernel needs to know the actual location
	  ) {
		//return orig_mmap(address, length, protect, flags, filedes, offset);
		// don't modify the arguments - we unfortunately can't touch this much, since it's going to be mapped in a way that we can't nicely diversify (yet)
	} else {
		is_diversified = true;

#if defined(MAP_ALIGN)
	// gotta round up to the next barrier, but this only matters on solaris (in theory)
	// support it as soon as it actually comes up
#error "MAP_ALIGN IS NOT YET SUPPORTED - IF YOU RUN INTO THIS CONGRATS, YOU HAVE SURPRISED ME"
#endif
		// we want to allocate a bit more space - this is so that each variant gets its own part of the allocated segment, in a non-overlapping fashion
		length = length * nnumvar;
	}
	// ok, now we need to actually do the call
	if(orig_mmap == NULL) {
		/*
		 * NOTE: WE CANNOT DO THIS - causes deadlock, infinite loops, and unpredictability
		 */
		//orig_mmap = (void*(*)(void*, size_t, int, int, int, off_t)) dlsym(RTLD_NEXT, "mmap");
		/*
		 * The reason is that if our _init function hasn't been called yet -
		 * which happens with surprising frequency - then the dlsym lookup
		 * eventually goes, acquires a lock, and calls mmap. This is bad, as
		 * that mmap call gets back here, and on the second pass, it deadlocks
		 * on the lock.
		 * Instead, we do essentially what the glibc mmap wrapper does here, and
		 * hope that no one is doing something esoteric that relies on any change
		 * in glibc's wrapper.
		 */
#ifndef SYSCALL_MMAP2_UNIT
#define SYSCALL_MMAP2_UNIT 4096ULL
#endif
#define UNIT SYSCALL_MMAP2_UNIT
#define OFF_MASK ((-0x2000ULL << (8*sizeof(long)-1)) | (UNIT-1))
		if (offset & OFF_MASK) {
			errno = EINVAL;
			return MAP_FAILED;
		}
		if (length >= PTRDIFF_MAX) {
			errno = ENOMEM;
			return MAP_FAILED;
		}
#ifdef SYS_mmap2
		new_mapping = (void*)syscall(SYS_mmap2, address, length, protect, flags, filedes, offset/UNIT);
#else
		new_mapping = (void*)syscall(SYS_mmap, address, length, protect, flags, filedes, offset);
#endif
	} else {
		// it's actually been initialized, so use the original glibc/other mmap implementation - let's hope it plays nice with the hacky stuff above
		new_mapping = orig_mmap(address, length, protect, flags, filedes, offset);
	}
	if(new_mapping == MAP_FAILED) {
		// it failed, and if we were to retry, it would diverge anyway - just return the failure
		return new_mapping;
	}
	if(is_diversified) {
		// we got the extra memory, now return the right part
		return new_mapping + originallength * nthisvar;
	} else {
		// we had to do just a normal mmap, so just return the pointer
		return new_mapping;
	}
}
/*void* mmap64(void* address, size_t length, int protect, int flags, int filedes, off64_t offset) {
	printf("FOOBAR64\n");
	if(orig_mmap64 == NULL) {
		// this is only going to come up in dlsym, I think, don't worry about it too much...
		return bad_decision_64;
		orig_mmap64 = (void*(*)(void*, size_t, int, int, int, off64_t)) dlsym(RTLD_DEFAULT, "mmap64");
	}
	return orig_mmap64(address, length, protect, flags, filedes, offset);
}*/
