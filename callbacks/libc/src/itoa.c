
#include <strlen.h>

void itox(unsigned int i, char *s)
{
    unsigned char n;
 
    s += 8;
    *s = '\0';
 
    for (n = 8; n != 0; --n) {
        *--s = "0123456789ABCDEF"[i & 0x0F];
        i >>= 4;
    }
}
