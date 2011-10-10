/*
Use of Incorrect Byte Ordering
Description Summary

The software receives input from an upstream component, but it does not account
for byte ordering (e.g. big-endian and little-endian) when processing the
input, causing an incorrect number or value to be used. 

from CERT
https://www.securecoding.cert.org/confluence/display/seccode/POS39-C.+Use+the+correct+byte+ordering+when+transferring+data+between+systems

Non-Compliant Code Example
In this noncompliant code example, the programmer tries to read an unsigned
32-bit integer off a previously connected network socket. 

It is important to know the sizes of your data types lest they be different on
architectures that are accessible over the network. Hence, we transfer a
uint32_t rather than an int. For more information, see recommendation FIO09-C.
Be careful with binary data when transferring data across systems.

@BAD_ARGS 17
@ATTACK_SUCCEEDED_CODE 1
@NORMAL_ERROR_CODE 0

*/

#include <stdio.h>
#include <stdlib.h>

// 2-byte number
int SHORT_little_endian_TO_big_endian(int i)
{
    return ((i>>8)&0xff)+((i << 8)&0xff00);
}

// 4-byte number
int INT_little_endian_TO_big_endian(int i)
{
    return((i&0xff)<<24)+((i&0xff00)<<8)+((i&0xff0000)>>8)+((i>>24)&0xff);
}

/* converters from Anghel Leonard devx.com */

/*
This program prints out the number received from the socket using an incorrect byte ordering. For example, if the value 4 is sent from a big endian machine, and the receiving system is little endian, the value 536,870,912 is read. This problem can be corrected by sending and receiving using network byte ordering.

Compliant Code Example
In this compliant code example, the programmer uses the ntohl() function to convert the integer from network byte order to host byte ordering. 
*/
/* sock is a connected TCP socket */

int getBigEndianInt(int num)
{
  return INT_little_endian_TO_big_endian(num);
}

int main(int argc, char **argv)
{

int num;
num = getBigEndianInt(atoi(argv[1]));

#ifdef SAFE
num = ntohl(num);
#endif

printf("We recieved %d from the network!\n", num);
if (num != atoi(argv[1])) exit(1);
exit(0);
}


/*
The ntohl() function (network to host long) translates a uint32_t value into
the host byte ordering from the network byte ordering. This function is always
appropriate to use because its implementation depends upon the specific systems
byte ordering. Consequently, on a big endian architecture, ntohl() does
nothing.  The reciprocal function htonl() (host to network long) should be used
before sending any data to another system over network protocols.

Portability Details:
ntohs(), ntohl(), htons(), and htonl() are not part of the C standard and are,
consequently, not guaranteed to be portable to non-POSIX systems.  The POSIX
implementations of ntohs(), ntohl(), htons() and htonl() take arguments of
types uint16_t and uint32_t and can be found in the header file <arpa/inet.h>.
The Windows implementations use unsigned short and unsigned long and can be
found in the header file <winsock2.h>.  Other variants of ntoht() and htont()
may exist on some systems, such as ntohi()/htoni() or ntohll()/htonll(). 
*/
