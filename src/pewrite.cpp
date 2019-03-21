
#include <zipr_all.h>
#include <irdb-core>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <assert.h>
#include <sys/mman.h>
#include <ctype.h>
#include <iostream>   
#include <string>     
#include <fstream>
#include <elf.h>


using namespace IRDB_SDK;
using namespace std;
using namespace zipr;
using namespace EXEIO;


static inline uintptr_t page_round_down(uintptr_t x)
{
        return x & (~(PAGE_SIZE-1));
}
static inline uintptr_t page_round_up(uintptr_t x)
{
        return  ( (((uintptr_t)(x)) + PAGE_SIZE-1)  & (~(PAGE_SIZE-1)) );
}

#if 0
/*
Note: all multi-byte values are stored LSB first. One block is 512 bytes, one paragraph is 16 bytes. See also the entry in Ralf Brown's Interrupt List

Offset (hex)
Meaning
00-01	0x4d, 0x5a. This is the "magic number" of an EXE file. The first byte of the file is 0x4d and the second is 0x5a.
02-03	The number of bytes in the last block of the program that are actually used. If this value is zero, that means the entire last block is used (i.e. the effective value is 512).
04-05	Number of blocks in the file that are part of the EXE file. If [02-03] is non-zero, only that much of the last block is used.
06-07	Number of relocation entries stored after the header. May be zero.
08-09	Number of paragraphs in the header. The program's data begins just after the header, and this field can be used to calculate the appropriate file offset. The header includes the relocation entries. Note that some OSs and/or programs may fail if the header is not a multiple of 512 bytes.
0A-0B	Number of paragraphs of additional memory that the program will need. This is the equivalent of the BSS size in a Unix program. The program can't be loaded if there isn't at least this much memory available to it.
0C-0D	Maximum number of paragraphs of additional memory. Normally, the OS reserves all the remaining conventional memory for your program, but you can limit it with this field.
0E-0F	Relative value of the stack segment. This value is added to the segment the program was loaded at, and the result is used to initialize the SS register.
10-11	Initial value of the SP register.
12-13	Word checksum. If set properly, the 16-bit sum of all words in the file should be zero. Usually, this isn't filled in.
14-15	Initial value of the IP register.
16-17	Initial value of the CS register, relative to the segment the program was loaded at.
18-19	Offset of the first relocation item in the file.
1A-1B	Overlay number. Normally zero, meaning that it's the main program.
Here is a structure that can be used to represent the EXE header and relocation entries, assuming a 16-bit LSB machine:
*/
struct EXE {
  unsigned short signature; /* == 0x5a4D */
  unsigned short bytes_in_last_block;
  unsigned short blocks_in_file;
  unsigned short num_relocs;
  unsigned short header_paragraphs;
  unsigned short min_extra_paragraphs;
  unsigned short max_extra_paragraphs;
  unsigned short ss;
  unsigned short sp;
  unsigned short checksum;
  unsigned short ip;
  unsigned short cs;
  unsigned short reloc_table_offset;
  unsigned short overlay_number;
};

struct EXE_RELOC {
  unsigned short offset;
  unsigned short segment;
};
/*
The offset of the beginning of the EXE data is computed like this:

exe_data_start = exe.header_paragraphs * 16L;
The offset of the byte just after the EXE data (in DJGPP, the size of the stub and the start of the COFF image) is computed like this:

extra_data_start = exe.blocks_in_file * 512L;
if (exe.bytes_in_last_block)
  extra_data_start -= (512 - exe.bytes_in_last_block);
  */
#endif
uint8_t dos_header[]= 
	{
	/* 00000000 */  0x4d, 0x5a, 0x90, 0x00, 0x03, 0x00, 0x00, 0x00,  0x04, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,  // |MZ..............|
	/* 00000010 */  0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // |........@.......|
	/* 00000020 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // |................|
	/* 00000030 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,  // |................|
	/* 00000040 */  0x0e, 0x1f, 0xba, 0x0e, 0x00, 0xb4, 0x09, 0xcd,  0x21, 0xb8, 0x01, 0x4c, 0xcd, 0x21, 0x54, 0x68,  // |........!..L.!Th|
	/* 00000050 */  0x69, 0x73, 0x20, 0x70, 0x72, 0x6f, 0x67, 0x72,  0x61, 0x6d, 0x20, 0x63, 0x61, 0x6e, 0x6e, 0x6f,  // |is program canno|
	/* 00000060 */  0x74, 0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6e,  0x20, 0x69, 0x6e, 0x20, 0x44, 0x4f, 0x53, 0x20,  // |t be run in DOS |
	/* 00000070 */  0x6d, 0x6f, 0x64, 0x65, 0x2e, 0x0d, 0x0d, 0x0a,  0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   // |mode....$.......|
	};
/* 
 * notes:
 *      0) A PE32+ file starts with a dos header (explained above).  This is mostly ignored, except the word at byte 0x3c.
 *	1) The 4-bytes at location 0x3c specify where the PE file header starts, this is the real meat of the heading.
 *	2) the PE file header comes next.
 */




void PeWriter::Write(const EXEIO::exeio *exeiop, const string &out_file, const string &infile)
{
//	const auto time_since_epoch=time(nullptr);

	assert(0); // to do 

}

