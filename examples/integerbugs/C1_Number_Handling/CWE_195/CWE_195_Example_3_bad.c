/*
Signed to Unsigned Conversion Error

Description Summary
A signed-to-unsigned conversion error takes place when a signed primitive is used 
as an unsigned value, usually as a size variable. 

Extended Description
It is dangerous to rely on implicit casts between signed and unsigned numbers because 
the result can take on an unexpected value and violate assumptions made by the program. 

Scope Effect 
Availability Conversion between signed and unsigned values can lead to a variety of 
errors, but from a security standpoint is most commonly associated with integer 
overflow and buffer overflow vulnerabilities.

Example 3
The following code is intended to read an incoming packet from a socket and extract 
one or more headers.
(Bad Code)Example Language: C 

@BAD_ARGS -1
@GOOD_ARGS 100
@ATTACK_SUCCEEDED_CODE 139

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>


/* Stupid stubs to pretend to do network trafic */
struct DataPacket {
  int headers;
};
typedef char PacketHeader;
int AcceptSocketConnection() {}
void ReadPacket(struct DataPacket* packet, int sock) { packet->headers = -3;}
void ParsePacketHeaders(struct DataPacket* packet, PacketHeader* header) {
   char *msg = "this is a long string that should crash the program";
   printf("%s\n", msg);
   strcpy(header, msg);
   printf("%s\n", header);
}


int main(int argc, char **argv)
{
  struct DataPacket *packet;
  int numHeaders;
  PacketHeader *headers;

  if (argc < 2) exit(2);

  int sock=AcceptSocketConnection();
  ReadPacket(packet, sock);
  packet->headers = atoi(argv[1]);
  numHeaders =packet->headers;

  if (numHeaders > 100) {
    exit(2);  /* too many headers! */
  }
  printf("%d\n", numHeaders);
  printf("%d\n", sizeof(PacketHeader));
  headers = malloc(numHeaders * sizeof(PacketHeader));
  ParsePacketHeaders(packet, headers);
  exit(0);
}

/*
The code performs a check to make sure that the packet does not contain too many 
headers. However, numHeaders is defined as a signed int, so it could be negative. 
If the incoming packet specifies a value such as -3, then the malloc calculation 
will generate a negative number (say, -300 if each header can be a maximum of 100 
bytes). When this result is provided to malloc(), it is first converted to a size_t 
type. This conversion then produces a large value such as 4294966996, which may 
cause malloc() to fail or to allocate an extremely large amount of memory (CWE-195). 
With the appropriate negative numbers, an attacker could trick malloc() into using 
a very small positive number, which then allocates a buffer that is much smaller 
than expected, potentially leading to a buffer overflow. 

*/

