/* mc2zk - rev_memcpy.c
 * 
 * An implementation of reverse memcpy()
 * based on GNU memcpy code from: 
 * ftp://alpha.gnu.org/gnu/coreutils/textutils-2.0f.tar.gz/textutils-2.0f/lib/
 *
 * Sample C source for memcpy()
 *
 * char *
 * memcpy (char *destaddr, const char *srcaddr, int len)
 * {
 *   char *dest = destaddr;
 *
 *     while (len-- > 0)
 *         *destaddr++ = *srcaddr++;
 *   return dest;
 * }
 *
 * rev_memcpy() will copy the memory in reverse order to destination.  
 * This will enable underflows.
 *
 */
#include "rev_memcpy.h"

void * rev_memcpy (void *destaddr, const void *srcaddr, int len)
{
	int *dest = destaddr;
	int const *src = srcaddr;
 
	while (len-- > 0)
		*dest-- = *src++;
	return destaddr;
}
