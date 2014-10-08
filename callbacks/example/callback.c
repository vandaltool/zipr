#include <stdint.h>

__attribute__ ((externally_visible)) __attribute__ ((used)) print_hello()
{
	char str[]="Hello";
	write(1,str,sizeof(str));
}

void truncation_detector_signed_32_8(uintptr_t ret)
{
	char str[]="truncation_detector_signed_32_8 called\n";
	write(1,str,sizeof(str));
}
