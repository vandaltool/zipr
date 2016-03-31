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
#include <fcntl.h>

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
 *
 * There's four modes that this mmap function defined here uses. The first mode,
 * which is slightly more complex, is for times when _init isn't run before mmap
 * is called for the first time. This happens in cases where an LD_PRELOADed .so
 * library calls mmap during its _init routine, such as in libheaprand.so. Since
 * it is extremely difficult/impossible to guarantee that we run first, and also
 * because dlsym requires mmap itself (and therefore deadlocks/infinite loops if
 * it's being used from inside an mmap call).
 *
 * The second mode, which is, in contrast, extremely simple, is for when an mmap
 * call has arguments that imply that our normal diversification technique would
 * cause improper program behavior. This is essentially just a pass-through mode
 * to the original glibc mmap, or our direct syscall of mmap in pre-init cases.
 *
 * The third mode is the normal diversification mode. This is pretty much just a
 * wrapper for the other mmaps that allocates a larger region, uses mprotect and
 * some math to disable all but a variant-indexed-part of it, and sends back the
 * pointer to the remaining part of the region. This ensures that every version,
 * regardless of index, doesn't overlap with any other version in this mode.
 *
 * The final mode is the probabalistic diversification mode. This mode, like the
 * deterministic non-overlapping-heap mode, allocates additional memory regions.
 * However, it selects the sub-region at pseudo-random, making it less likely an
 * address will be mapped the same way in multiple variants. This is really just
 * a fallback, though, since it's likely that a good number will overlap.
 */

int nthisvar = 0;
int nnumvar  = 1;
int randfd   = 0;
int pagesize = 0;

// yeah, it's a lot of extra allocation, but it's all virtual allocations that are immediately
// paged out - not actually a problem long-term in a 64-bit address space (generally)
#define PROB_NUM_VARIANTS 64

extern void* mmap(void*, size_t, int, int, int, off_t);

void* (*orig_mmap)(void*, size_t, int, int ,int, off_t) = NULL;

int get_config_vars(void) {
	int fd = open("/variant_specific/nolnoh_config",O_RDONLY);
	if(fd == -1)
		return -1;
	int bufsize = 17;
	char buf[bufsize];
	int i;
	for(i=0;i<bufsize;i++) {
		buf[i]='\0';
	}
	ssize_t nr = read(fd, buf, bufsize-1);
	if(nr == 0) {
		// we read 0 of the tokens from the file, since we read absolutely nothing
		close(fd);
		return 0;
	}
	nnumvar = atoi(buf);
	// note that this currently caps at 512 variants, increase this number if necessary to support more
	if(nnumvar <= 0 || nnumvar > 512) {
		// wasn't a valid value
		nnumvar = 0;
		close(fd);
		return 0;
	}
	int mode = 0;
	// get the second number
	for(i=0;i<bufsize;i++) {
		if(buf[i] == '\0') {
			close(fd);
			return 1;
		}
		if(buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\r' || buf[i] == '\n') {
			mode = 1;
		}
		if(mode == 1 && buf[i] >= '0' && buf[i] <= '9') {
			nthisvar = atoi(buf+i);
			// now sanity check it
			if(nthisvar >= nnumvar || nthisvar < 0) {
				nthisvar = 0;
				close(fd);
				return 1;
			}
			close(fd);
			return 2;
		}
	}
	// we reached the end of the string in an unexpected manner; just say that the thing we have works
	close(fd);
	return 1;
}

void _init(void) {
	orig_mmap = (void*(*)(void*, size_t, int, int, int, off_t)) dlsym(RTLD_NEXT, "mmap");
	int parsed = get_config_vars();
#ifdef DEBUG
#ifdef MVEE_SAFE
	printf("parsed was X\n");
#else
	printf("parsed was %i\n",parsed);
#endif
#endif
	if(parsed <= 0) {
		//neither nnumvar nor nthivar are set
		// if we don't know how many variants, try doing randomization of up to 64
		if(randfd == 0) {
			// run in probabalistic mode, since we didn't find the arguments for proper indexing
			randfd = open("/dev/cfar_urandom",O_RDONLY);
		}
		// we're either going to use nthisvar in the ultimate failsafe mode where we don't do the randomization, or we're going to ignore it; either way, it's 0
		nthisvar = 0;
		// now check if it's still 0, or if it actually set up properly this time
		if(randfd != 0) {
			// we don't use nthisvar, just nnumvar and a random choice
			nnumvar = PROB_NUM_VARIANTS;
		} else {
			// if we don't have our randomness source, and we don't have structured info, just behave as normal mmap
			// (with the two extra mprotect calls, so syscall alignment is preserved)
			nnumvar = 1;
		}
	} else if (parsed == 1) {
		//nnumvar is set, but not nthisvar
		// if we don't know how many variants, try doing randomization of up to 64
		if(randfd == 0) {
			// run in probabalistic mode, since we didn't find the arguments for proper indexing
			randfd = open("/dev/cfar_urandom",O_RDONLY);
		}
		// we're either going to use nthisvar in the ultimate failsafe mode where we don't do the randomization, or we're going to ignore it; either way, it's 0
		nthisvar = 0;
		// if randfd is still 0, we just default to the safe case; in any case, we're done here
	} else if (parsed == 2) {
		//nthisvar and nnumvar have been set, we're done here
	}
#ifdef DEBUG
	printf("finished _init\n");
#endif
}

// rounding up to alignments is important and useful
size_t rounduptomultiple(size_t length, int roundto) {
	size_t remainder = length % roundto;
	if(remainder == 0) {
		return length;
	}
	return length + roundto - remainder;
}

// we need page alignment for a few things - mmap works much better with it
size_t rounduptopagemultiple(size_t length) {
	if(pagesize == 0) {
		pagesize = getpagesize();
	}
	return rounduptomultiple(length, pagesize);
}

// the non-diversified part of the mapping - this basically acts as a call to mmap that works in a couple of odd additional contexts
void* actually_mmap(void* address, size_t length, int protect, int flags, int filedes, off_t offset) {
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
		return (void*)syscall(SYS_mmap2, address, length, protect, flags, filedes, offset/UNIT);
#else
		return (void*)syscall(SYS_mmap, address, length, protect, flags, filedes, offset);
#endif
	} else {
		// it's actually been initialized, so use the original glibc/other mmap implementation - let's hope it plays nice with the hacky stuff above
		return orig_mmap(address, length, protect, flags, filedes, offset);
	}
}

void* mmap(void* address, size_t length, int protect, int flags, int filedes, off_t offset) {
#ifdef DEBUG
	printf("entering mmap call\n");
#endif
	// figure out what index we are in a randomized context

	if(
			(flags & MAP_FIXED)	// must use normal mmap, since the program is now guaranteed the destination address or failure
			|| (flags & MAP_SHARED)			// we're sharing between multiple programs, which is complex enough as it is - alignment is important, and we'd have to do funky stuff to ensure non-overlappingness with this regardless
	  ) {
#ifdef DEBUG
		printf("non-modified path\n");
#endif
		// don't modify the arguments - we unfortunately can't touch this much, since it's going to be mapped in a way that we can't nicely diversify (yet)
		return actually_mmap(address, length, protect, flags, filedes, offset);
	} else {
		// we're diversified now - do fancier stuff
		size_t alignedlength = length;
		void* new_mapping = MAP_FAILED;
		// get the nthisvar for this run
		if(randfd != 0 && randfd != -1) {
#ifdef DEBUG
#ifdef MVEE_SAFE
			printf("using randomization, is at X/X\n");
#else
			printf("using randomization, is at %i/%i\n",nthisvar,nnumvar);
#endif
#endif
			// we're in probabalistic mode
			uint32_t target = 0;
			read(randfd, &target, 4);
			nthisvar = (int)(target % nnumvar);
#ifdef DEBUG
#ifdef MVEE_SAFE
			printf("chose X/X\n");
#else
			printf("chose %i/%i\n",nthisvar,nnumvar);
#endif
#endif
		}
		// branch on whether this is a file-backed allocation or not
		if (flags & MAP_ANONYMOUS) {
#ifdef DEBUG
			printf("using anonymous mapping path\n");
#endif
			// if it's a non-file mapping, just allocate a larger area, and then use part of that
#if defined(MAP_ALIGN)
			// gotta round up to the next barrier, but this only matters on solaris (in theory)
			// support it as soon as it actually comes up
			length = rounduptomultiple(length, address);
#endif
			// we want to allocate a bit more space - this is so that each variant gets its own part of the allocated segment, in a non-overlapping fashion
			// let's keep page alignment the same
			alignedlength = rounduptopagemultiple(length);
			length = alignedlength * nnumvar;
			new_mapping = actually_mmap(address, length, protect, flags, filedes, offset);

			// now that we've done the call, handle the result and return
			if(new_mapping == MAP_FAILED) {
				// it failed, and if we were to retry, it would diverge anyway - just return the failure
				// alternatively, we had to do just a normal mmap, so just return the pointer
				return new_mapping;
			}

			// in the event that we got the memory and such, mprotect the other portions so that they won't be able to be accesssed improperly
			mprotect(new_mapping, alignedlength * nthisvar, PROT_NONE);
			mprotect(new_mapping + alignedlength * (nthisvar+1), alignedlength * (nnumvar - (nthisvar + 1)), PROT_NONE);
#ifdef DEBUG
#ifdef MVEE_SAFE
			printf("returning new mapping at X\n");
#else
			printf("returning new mapping at %p\n", new_mapping + alignedlength * nthisvar);
#endif
#endif
			return new_mapping + alignedlength * nthisvar;
		} else {
#ifdef DEBUG
			printf("performing tricky file mapping approach\n");
#endif
			// for file mappings, handle them by repeated allocation of the required size and selection of the currect index
			int i=0;
			for(i=0; i < nnumvar; i++) {
				if(i == nthisvar) {
					new_mapping = actually_mmap(address, length, protect, flags, filedes, offset);
				} else {
					// ignored
					actually_mmap(address, length, PROT_NONE, flags | MAP_ANONYMOUS, -1, 0);
				}
			}
			return new_mapping;
		}
	}
}
