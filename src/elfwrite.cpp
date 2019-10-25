
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
#include <iostream>   // std::cout
#include <string>     // std::string, std::to_string
#include <fstream>
#include <elf.h>
#include <endian.h>
    
#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"

using namespace IRDB_SDK;
using namespace std;
using namespace zipr;
using namespace ELFIO;

template<class T_Elf_Ehdr, class T_Elf_Shdr>
static inline void host_to_shdr(const T_Elf_Ehdr& ehdr, T_Elf_Shdr& shdr)
{
#if 0
	struct Elf64_Shdr
	{
		Elf64_Word 	sh_name
		Elf64_Word 	sh_type
		Elf64_Xword 	sh_flags
		Elf64_Addr 	sh_addr
		Elf64_Off 	sh_offset
		Elf64_Xword 	sh_size
		Elf64_Word 	sh_link
		Elf64_Word 	sh_info
		Elf64_Xword 	sh_addralign
		Elf64_Xword 	sh_entsize
	}

	steruct Elf32_Shdr
	{
		Elf32_Word 	sh_name
		Elf32_Word 	sh_type
		Elf32_Word 	sh_flags
		Elf32_Addr 	sh_addr
		Elf32_Off 	sh_offset
		Elf32_Word 	sh_size
		Elf32_Word 	sh_link
		Elf32_Word 	sh_info
		Elf32_Word 	sh_addralign
		Elf32_Word 	sh_entsize
	}
#endif


	if(ehdr.e_ident[5]==1)	 // little endian elf file
	{
		shdr.sh_name      = htole32(shdr.sh_name);
		shdr.sh_type      = htole32(shdr.sh_type);
		shdr.sh_link      = htole32(shdr.sh_link);
		shdr.sh_info      = htole32(shdr.sh_info);
		if(sizeof(ehdr.e_entry)==4)	 // 32-bit header
		{
			shdr.sh_flags     = htole32(shdr.sh_flags);
			shdr.sh_size      = htole32(shdr.sh_size);
			shdr.sh_addr      = htole32(shdr.sh_addr);
			shdr.sh_offset    = htole32(shdr.sh_offset);
			shdr.sh_addralign = htole32(shdr.sh_addralign);
			shdr.sh_entsize   = htole32(shdr.sh_entsize);
		}
		else if(sizeof(ehdr.e_entry)==8)	// 64-bit header
		{
			shdr.sh_flags     = htole64(shdr.sh_flags);
			shdr.sh_size      = htole64(shdr.sh_size);
			shdr.sh_addr      = htole64(shdr.sh_addr);
			shdr.sh_offset    = htole64(shdr.sh_offset);
			shdr.sh_addralign = htole64(shdr.sh_addralign);
			shdr.sh_entsize   = htole64(shdr.sh_entsize);
		}
		else
		{
			assert(0);
		}

	}
	else if(ehdr.e_ident[5]==2) // big endian elf file
	{
		shdr.sh_name      = htobe32(shdr.sh_name);
		shdr.sh_type      = htobe32(shdr.sh_type);
		shdr.sh_link      = htobe32(shdr.sh_link);
		shdr.sh_info      = htobe32(shdr.sh_info);
		if(sizeof(ehdr.e_entry)==4)	 // 32-bit header
		{
			shdr.sh_flags     = htobe32(shdr.sh_flags);
			shdr.sh_size      = htobe32(shdr.sh_size);
			shdr.sh_addr      = htobe32(shdr.sh_addr);
			shdr.sh_offset    = htobe32(shdr.sh_offset);
			shdr.sh_addralign = htobe32(shdr.sh_addralign);
			shdr.sh_entsize   = htobe32(shdr.sh_entsize);
		}
		else if(sizeof(ehdr.e_entry)==8)	// 64-bit header
		{
			shdr.sh_flags     = htobe64(shdr.sh_flags);
			shdr.sh_size      = htobe64(shdr.sh_size);
			shdr.sh_addr      = htobe64(shdr.sh_addr);
			shdr.sh_offset    = htobe64(shdr.sh_offset);
			shdr.sh_addralign = htobe64(shdr.sh_addralign);
			shdr.sh_entsize   = htobe64(shdr.sh_entsize);
		}
		else
		{
			assert(0);
		}
	}
	else
	{
		assert(0);
	}

}

template<class T_Elf_Ehdr, class T_Elf_Shdr>
static inline void shdr_to_host(const T_Elf_Ehdr& ehdr, T_Elf_Shdr& shdr)
{

#if 0
	struct Elf64_Shdr
	{
		Elf64_Word 	sh_name
		Elf64_Word 	sh_type
		Elf64_Xword 	sh_flags
		Elf64_Addr 	sh_addr
		Elf64_Off 	sh_offset
		Elf64_Xword 	sh_size
		Elf64_Word 	sh_link
		Elf64_Word 	sh_info
		Elf64_Xword 	sh_addralign
		Elf64_Xword 	sh_entsize
	}

	steruct Elf32_Shdr
	{
		Elf32_Word 	sh_name
		Elf32_Word 	sh_type
		Elf32_Word 	sh_flags
		Elf32_Addr 	sh_addr
		Elf32_Off 	sh_offset
		Elf32_Word 	sh_size
		Elf32_Word 	sh_link
		Elf32_Word 	sh_info
		Elf32_Word 	sh_addralign
		Elf32_Word 	sh_entsize
	}
#endif


	if(ehdr.e_ident[5]==1)	 // little endian elf file
	{
		shdr.sh_name      = le32toh(shdr.sh_name);
		shdr.sh_type      = le32toh(shdr.sh_type);
		shdr.sh_link      = le32toh(shdr.sh_link);
		shdr.sh_info      = le32toh(shdr.sh_info);
		if(sizeof(ehdr.e_entry)==4)	 // 32-bit header
		{
			shdr.sh_flags     = le32toh(shdr.sh_flags);
			shdr.sh_size      = le32toh(shdr.sh_size);
			shdr.sh_addr      = le32toh(shdr.sh_addr);
			shdr.sh_offset    = le32toh(shdr.sh_offset);
			shdr.sh_addralign = le32toh(shdr.sh_addralign);
			shdr.sh_entsize   = le32toh(shdr.sh_entsize);
		}
		else if(sizeof(ehdr.e_entry)==8)	// 64-bit header
		{
			shdr.sh_flags     = le64toh(shdr.sh_flags);
			shdr.sh_size      = le64toh(shdr.sh_size);
			shdr.sh_addr      = le64toh(shdr.sh_addr);
			shdr.sh_offset    = le64toh(shdr.sh_offset);
			shdr.sh_addralign = le64toh(shdr.sh_addralign);
			shdr.sh_entsize   = le64toh(shdr.sh_entsize);
		}
		else
		{
			assert(0);
		}

	}
	else if(ehdr.e_ident[5]==2) // big endian elf file
	{
		shdr.sh_name      = be32toh(shdr.sh_name);
		shdr.sh_type      = be32toh(shdr.sh_type);
		shdr.sh_link      = be32toh(shdr.sh_link);
		shdr.sh_info      = be32toh(shdr.sh_info);
		if(sizeof(ehdr.e_entry)==4)	 // 32-bit header
		{
			shdr.sh_flags     = be32toh(shdr.sh_flags);
			shdr.sh_size      = be32toh(shdr.sh_size);
			shdr.sh_addr      = be32toh(shdr.sh_addr);
			shdr.sh_offset    = be32toh(shdr.sh_offset);
			shdr.sh_addralign = be32toh(shdr.sh_addralign);
			shdr.sh_entsize   = be32toh(shdr.sh_entsize);
		}
		else if(sizeof(ehdr.e_entry)==8)	// 64-bit header
		{
			shdr.sh_flags     = be64toh(shdr.sh_flags);
			shdr.sh_size      = be64toh(shdr.sh_size);
			shdr.sh_addr      = be64toh(shdr.sh_addr);
			shdr.sh_offset    = be64toh(shdr.sh_offset);
			shdr.sh_addralign = be64toh(shdr.sh_addralign);
			shdr.sh_entsize   = be64toh(shdr.sh_entsize);
		}
		else
		{
			assert(0);
		}
	}
	else
	{
		assert(0);
	}

}

template<class T_Elf_Ehdr, class T_Elf_Phdr>
static inline void phdr_to_host(const T_Elf_Ehdr& ehdr, T_Elf_Phdr& phdr)
{

#if 0

	64-bit:

	struct {
		Elf64_Word p_type;    size=4
		Elf64_Word p_flags;   size=4
		Elf64_Off p_offset;   size=8
		Elf64_Addr p_vaddr;   size=8
		Elf64_Addr p_paddr;   size=8
		Elf64_Xword p_filesz; size=8
		Elf64_Xword p_memsz;  size=8
		Elf64_Xword p_align;  size=8
	}

	32-bit: 

	struct {
		Elf32_Word p_type;    4
		Elf32_Off p_offset;   4
		Elf32_Addr p_vaddr;   4
		Elf32_Addr p_paddr;   4
		Elf32_Word p_filesz;  4
		Elf32_Word p_memsz;   4
		Elf32_Word p_flags;   4
		Elf32_Word p_align;   4
	}
#endif


	if(ehdr.e_ident[5]==1)	 // little endian elf file
	{
		phdr.p_flags = le32toh(phdr.p_flags);
		if(sizeof(ehdr.e_entry)==4)	 // 32-bit header
		{
			phdr.p_offset = le32toh(phdr.p_offset);
			phdr.p_vaddr  = le32toh(phdr.p_vaddr);
			phdr.p_paddr  = le32toh(phdr.p_paddr);
			phdr.p_filesz = le32toh(phdr.p_filesz);
			phdr.p_memsz  = le32toh(phdr.p_memsz);
			phdr.p_align  = le32toh(phdr.p_align);
		}
		else if(sizeof(ehdr.e_entry)==8)	// 64-bit header
		{
			phdr.p_offset = le64toh(phdr.p_offset);
			phdr.p_vaddr  = le64toh(phdr.p_vaddr);
			phdr.p_paddr  = le64toh(phdr.p_paddr);
			phdr.p_filesz = le64toh(phdr.p_filesz);
			phdr.p_memsz  = le64toh(phdr.p_memsz);
			phdr.p_align  = le64toh(phdr.p_align);
		}
		else
		{
			assert(0);
		}

	}
	else if(ehdr.e_ident[5]==2) // big endian elf file
	{
		phdr.p_type = be32toh(phdr.p_type);
		phdr.p_flags = be32toh(phdr.p_flags);
		if(sizeof(ehdr.e_entry)==4)	 // 32-bit header
		{
			phdr.p_offset = be32toh(phdr.p_offset);
			phdr.p_vaddr  = be32toh(phdr.p_vaddr);
			phdr.p_paddr  = be32toh(phdr.p_paddr);
			phdr.p_filesz = be32toh(phdr.p_filesz);
			phdr.p_memsz  = be32toh(phdr.p_memsz);
			phdr.p_align  = be32toh(phdr.p_align);
		}
		else if(sizeof(ehdr.e_entry)==8)	// 64-bit header
		{
			phdr.p_offset = be64toh(phdr.p_offset);
			phdr.p_vaddr  = be64toh(phdr.p_vaddr);
			phdr.p_paddr  = be64toh(phdr.p_paddr);
			phdr.p_filesz = be64toh(phdr.p_filesz);
			phdr.p_memsz  = be64toh(phdr.p_memsz);
			phdr.p_align  = be64toh(phdr.p_align);
		}
		else
		{
			assert(0);
		}

	}
	else
	{
		assert(0);
	}

}

template<class T_Elf_Ehdr, class T_Elf_Phdr>
static inline void host_to_phdr(const T_Elf_Ehdr& ehdr, T_Elf_Phdr& phdr)
{

#if 0

	64-bit:

	struct {
		Elf64_Word p_type;    size=4
		Elf64_Word p_flags;   size=4
		Elf64_Off p_offset;   size=8
		Elf64_Addr p_vaddr;   size=8
		Elf64_Addr p_paddr;   size=8
		Elf64_Xword p_filesz; size=8
		Elf64_Xword p_memsz;  size=8
		Elf64_Xword p_align;  size=8
	}

	32-bit: 

	struct {
		Elf32_Word p_type;    4
		Elf32_Off p_offset;   4
		Elf32_Addr p_vaddr;   4
		Elf32_Addr p_paddr;   4
		Elf32_Word p_filesz;  4
		Elf32_Word p_memsz;   4
		Elf32_Word p_flags;   4
		Elf32_Word p_align;   4
	}
#endif


	if(ehdr.e_ident[5]==1)	 // little endian elf file
	{
		phdr.p_flags = htole32(phdr.p_flags);
		if(sizeof(ehdr.e_entry)==4)	 // 32-bit header
		{
			phdr.p_offset = htole32(phdr.p_offset);
			phdr.p_vaddr  = htole32(phdr.p_vaddr);
			phdr.p_paddr  = htole32(phdr.p_paddr);
			phdr.p_filesz = htole32(phdr.p_filesz);
			phdr.p_memsz  = htole32(phdr.p_memsz);
			phdr.p_align  = htole32(phdr.p_align);
		}
		else if(sizeof(ehdr.e_entry)==8)	// 64-bit header
		{
			phdr.p_offset = htole64(phdr.p_offset);
			phdr.p_vaddr  = htole64(phdr.p_vaddr);
			phdr.p_paddr  = htole64(phdr.p_paddr);
			phdr.p_filesz = htole64(phdr.p_filesz);
			phdr.p_memsz  = htole64(phdr.p_memsz);
			phdr.p_align  = htole64(phdr.p_align);
		}
		else
		{
			assert(0);
		}

	}
	else if(ehdr.e_ident[5]==2) // big endian elf file
	{
		phdr.p_type = htobe32(phdr.p_type);
		phdr.p_flags = htobe32(phdr.p_flags);
		if(sizeof(ehdr.e_entry)==4)	 // 32-bit header
		{
			phdr.p_offset = htobe32(phdr.p_offset);
			phdr.p_vaddr  = htobe32(phdr.p_vaddr);
			phdr.p_paddr  = htobe32(phdr.p_paddr);
			phdr.p_filesz = htobe32(phdr.p_filesz);
			phdr.p_memsz  = htobe32(phdr.p_memsz);
			phdr.p_align  = htobe32(phdr.p_align);
		}
		else if(sizeof(ehdr.e_entry)==8)	// 64-bit header
		{
			phdr.p_offset = htobe64(phdr.p_offset);
			phdr.p_vaddr  = htobe64(phdr.p_vaddr);
			phdr.p_paddr  = htobe64(phdr.p_paddr);
			phdr.p_filesz = htobe64(phdr.p_filesz);
			phdr.p_memsz  = htobe64(phdr.p_memsz);
			phdr.p_align  = htobe64(phdr.p_align);
		}
		else
		{
			assert(0);
		}

	}
	else
	{
		assert(0);
	}

}

template<class T_Elf_Ehdr>
static inline void ehdr_to_host(T_Elf_Ehdr& ehdr)
{
#if 0
struct Elf32_Ehdr {
    unsigned char e_ident[16];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
}
struct Elf64_Ehdr {
    unsigned char e_ident[16];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off e_phoff;
    Elf64_Off e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
}
#endif

	if(ehdr.e_ident[5]==1)	 // little endian elf file
	{
		ehdr.e_type       = le16toh(ehdr.e_type);
		ehdr.e_machine    = le16toh(ehdr.e_machine);
		ehdr.e_version    = le32toh(ehdr.e_version);
		ehdr.e_flags      = le32toh(ehdr.e_flags);
		ehdr.e_ehsize     = le16toh(ehdr.e_ehsize);
		ehdr.e_phentsize  = le16toh(ehdr.e_phentsize);
		ehdr.e_phnum      = le16toh(ehdr.e_phnum);
		ehdr.e_shentsize  = le16toh(ehdr.e_shentsize);
		ehdr.e_shnum      = le16toh(ehdr.e_shnum);
		ehdr.e_shstrndx   = le16toh(ehdr.e_shstrndx);

		if(sizeof(ehdr.e_entry)==4)	 // 32-bit header
		{
			ehdr.e_entry = le32toh(ehdr.e_entry);
			ehdr.e_phoff = le32toh(ehdr.e_phoff);
			ehdr.e_shoff = le32toh(ehdr.e_shoff);
		}
		else if(sizeof(ehdr.e_entry)==8)	// 64-bit header
		{
			ehdr.e_entry = le64toh(ehdr.e_entry);
			ehdr.e_phoff = le64toh(ehdr.e_phoff);
			ehdr.e_shoff = le64toh(ehdr.e_shoff);
		}
		else
		{
			assert(0);
		}

	}
	else if(ehdr.e_ident[5]==2) // big endian elf file
	{
		ehdr.e_type       = be16toh(ehdr.e_type);
		ehdr.e_machine    = be16toh(ehdr.e_machine);
		ehdr.e_version    = be32toh(ehdr.e_version);
		ehdr.e_flags      = be16toh(ehdr.e_flags);
		ehdr.e_ehsize     = be32toh(ehdr.e_ehsize);
		ehdr.e_phentsize  = be16toh(ehdr.e_phentsize);
		ehdr.e_phnum      = be16toh(ehdr.e_phnum);
		ehdr.e_shentsize  = be16toh(ehdr.e_shentsize);
		ehdr.e_shnum      = be16toh(ehdr.e_shnum);
		ehdr.e_shstrndx   = be16toh(ehdr.e_shstrndx);

		if(sizeof(ehdr.e_entry)==4)	 //32-bit header
		{
			ehdr.e_entry = be32toh(ehdr.e_entry);
			ehdr.e_phoff = be32toh(ehdr.e_phoff);
			ehdr.e_shoff = be32toh(ehdr.e_shoff);
		}
		else if(sizeof(ehdr.e_entry)==8) // 64-bit header
		{
			ehdr.e_entry = be64toh(ehdr.e_entry);
			ehdr.e_phoff = be64toh(ehdr.e_phoff);
			ehdr.e_shoff = be64toh(ehdr.e_shoff);
		}
		else
		{
			assert(0);
		}

	}
	else
	{
		assert(0);
	}
}

template<class T_Elf_Ehdr>
static inline void host_to_ehdr(T_Elf_Ehdr& ehdr)
{
#if 0
struct Elf32_Ehdr {
    unsigned char e_ident[16];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
}
struct Elf64_Ehdr {
    unsigned char e_ident[16];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off e_phoff;
    Elf64_Off e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
}
#endif
	if(ehdr.e_ident[5]==1)	 // little endian elf file
	{
		ehdr.e_type       = htole16(ehdr.e_type);
		ehdr.e_machine    = htole16(ehdr.e_machine);
		ehdr.e_version    = htole32(ehdr.e_version);
		ehdr.e_flags      = htole32(ehdr.e_flags);
		ehdr.e_ehsize     = htole16(ehdr.e_ehsize);
		ehdr.e_phentsize  = htole16(ehdr.e_phentsize);
		ehdr.e_phnum      = htole16(ehdr.e_phnum);
		ehdr.e_shentsize  = htole16(ehdr.e_shentsize);
		ehdr.e_shnum      = htole16(ehdr.e_shnum);
		ehdr.e_shstrndx   = htole16(ehdr.e_shstrndx);

		if(sizeof(ehdr.e_entry)==4)	 // 32-bit header
		{
			ehdr.e_entry = htole32(ehdr.e_entry);
			ehdr.e_phoff = htole32(ehdr.e_phoff);
			ehdr.e_shoff = htole32(ehdr.e_shoff);
		}
		else if(sizeof(ehdr.e_entry)==8)	// 64-bit header
		{
			ehdr.e_entry = htole64(ehdr.e_entry);
			ehdr.e_phoff = htole64(ehdr.e_phoff);
			ehdr.e_shoff = htole64(ehdr.e_shoff);
		}
		else
		{
			assert(0);
		}

	}
	else if(ehdr.e_ident[5]==2) // big endian elf file
	{
		ehdr.e_type       = htobe16(ehdr.e_type);
		ehdr.e_machine    = htobe16(ehdr.e_machine);
		ehdr.e_version    = htobe32(ehdr.e_version);
		ehdr.e_flags      = htobe32(ehdr.e_flags);
		ehdr.e_ehsize     = htobe16(ehdr.e_ehsize);
		ehdr.e_phentsize  = htobe16(ehdr.e_phentsize);
		ehdr.e_phnum      = htobe16(ehdr.e_phnum);
		ehdr.e_shentsize  = htobe16(ehdr.e_shentsize);
		ehdr.e_shnum      = htobe16(ehdr.e_shnum);
		ehdr.e_shstrndx   = htobe16(ehdr.e_shstrndx);

		if(sizeof(ehdr.e_entry)==4)	 //32-bit header
		{
			ehdr.e_entry = htobe32(ehdr.e_entry);
			ehdr.e_phoff = htobe32(ehdr.e_phoff);
			ehdr.e_shoff = htobe32(ehdr.e_shoff);
		}
		else if(sizeof(ehdr.e_entry)==8) // 64-bit header
		{
			ehdr.e_entry = htobe64(ehdr.e_entry);
			ehdr.e_phoff = htobe64(ehdr.e_phoff);
			ehdr.e_shoff = htobe64(ehdr.e_shoff);
		}
		else
		{
			assert(0);
		}

	}
	else
	{
		assert(0);
	}
}

static inline uintptr_t page_round_down(uintptr_t x)
{
	return x & (~(PAGE_SIZE-1));
}
static inline uintptr_t page_round_up(uintptr_t x)
{
        return  ( (((uintptr_t)(x)) + PAGE_SIZE-1)  & (~(PAGE_SIZE-1)) );
}

void ElfWriter::Write(const string &out_file, const string &infile)
{
	auto fin=fopen(infile.c_str(), "r");
	auto fout=fopen(out_file.c_str(), "w");
	assert(fin && fout);

	CreatePagemap();
	CreateSegmap();
	//SortSegmap();
	VirtualOffset_t min_addr=DetectMinAddr();
	VirtualOffset_t max_addr=DetectMaxAddr();

	LoadEhdr(fin);
	LoadPhdrs(fin);
	CreateNewPhdrs(min_addr,max_addr);


	WriteElf(fout);

	if( m_write_sections ) 
		AddSections(fout);

}

VirtualOffset_t ExeWriter::DetectMinAddr()
{
	auto firp=m_firp;
	VirtualOffset_t min_addr=(*(firp->getDataScoops().begin()))->getStart()->getVirtualOffset();
	for(DataScoopSet_t::iterator it=firp->getDataScoops().begin(); it!=firp->getDataScoops().end(); ++it)
	{
		DataScoop_t* scoop=*it;

		if(scoop->getStart()->getVirtualOffset() < min_addr)
			min_addr=scoop->getStart()->getVirtualOffset();

	}
	return min_addr;

}

VirtualOffset_t ExeWriter::DetectMaxAddr()
{
	auto firp=m_firp;
	VirtualOffset_t max_addr=(*(firp->getDataScoops().begin()))->getEnd()->getVirtualOffset();
	for(DataScoopSet_t::iterator it=firp->getDataScoops().begin(); it!=firp->getDataScoops().end(); ++it)
	{
		DataScoop_t* scoop=*it;

		if(scoop->getEnd()->getVirtualOffset() > max_addr)
			max_addr=scoop->getEnd()->getVirtualOffset();

	}
	return max_addr;

}

void ExeWriter::CreatePagemap()
{
	auto firp=m_firp;

// 	for(DataScoopSet_t::iterator it=firp->getDataScoops().begin(); it!=firp->getDataScoops().end(); ++it)
// 		DataScoop_t* scoop=*it;
//
	for(auto scoop : firp->getDataScoops())
	{
		// tbss is an elf-byproduct that irdb doesn't entirely support.  
		// IRDB needs a better mechanism.
		// To support this for now, we can just ignore it here.
		if(scoop->getName()==".tbss")
			continue;

		auto scoop_addr=scoop->getStart();
		auto start_addr=scoop_addr->getVirtualOffset();
		auto end_addr=scoop->getEnd()->getVirtualOffset();

		// we'll deal with unpinned scoops later.
		if(scoop_addr->getVirtualOffset()==0)
		{
			assert(0); // none for now?
			continue;
		}

		for(VirtualOffset_t i=page_align(start_addr); i<=end_addr; i+=PAGE_SIZE)
		{
			PageData_t &pagemap_i=pagemap[i];
			//cout<<"Writing scoop "<<scoop->getName()<<" to page: "<<hex<<i<<", perms="<<scoop->getRawPerms()
			//    << " start="<<hex<< scoop->getStart()->getVirtualOffset()
			//    << " end="<<hex<< scoop->getEnd()->getVirtualOffset() <<endl;
			pagemap_i.union_permissions(scoop->getRawPerms());
			pagemap_i.is_relro |= scoop->isRelRo();
			for(int j=0;j<PAGE_SIZE;j++)
			{
				if(i+j < start_addr)
					continue;

				if(i+j > end_addr)
					continue;

				// get the data out of the scoop and put it into the page map.
				VirtualOffset_t offset=i+j-start_addr;
				if(offset<scoop->getContents().size())
				{
					// cout<<"Updating page["<<hex<<i<<"+"<<j<<"("<<(i+j)<<")]="<<hex<<(int)scoop->getContents()[ offset ]<<endl; 
					pagemap_i.data[j]=scoop->getContents()[ offset ]; 
					pagemap_i.inuse[j]=true;
				}
			}
		}

	}
}

void ExeWriter::SortSegmap()
{
	// do one interation of a bubble sort to move the segement with the largest bss last.
	for (unsigned int i=0; i<segvec.size()-1;i++)
	{
		int this_bss_size=segvec[i]->memsz-segvec[i]->filesz;
		int next_bss_size=segvec[i+1]->memsz-segvec[i+1]->filesz;

		if(this_bss_size > next_bss_size)
		{
			std::swap(segvec[i],segvec[i+1]);
		}

	}
}

void ExeWriter::CreateSegmap()
{
	const auto should_bss_optimize= [&] (const PageData_t& perms)
	{
		return (perms.is_zero_initialized() && m_bss_opts);
	};



	// init some segment vars.
	auto segstart=pagemap.begin()->first;
	auto segperms=pagemap.begin()->second;
	auto segend=segstart+PAGE_SIZE;
	auto initend=segstart;

	const auto update_initend=[&](const PageData_t& perms)
	{
		if(should_bss_optimize(perms))
			initend=segstart;
		else
			initend=segend;
	};

	update_initend(segperms);

	auto it=pagemap.begin(); 
	++it;	// handled first one above.

	for( /* init'd above */; it!=pagemap.end(); ++it)
	{
		// grab page address and perms
		const auto pagestart=it->first;
		const auto &perms=it->second;


		// if we switch perms, or skip a page 
		if( (perms.m_perms!=segperms.m_perms) || (segend!=pagestart))
		{
			const auto seg=new LoadSegment_t(initend-segstart, segend-segstart, 0, segstart,segperms.m_perms);
			segvec.push_back(seg);

			cout<<"Found segment "<<hex<<segstart<<"-"<<(segend-1)<<", perms="<<segperms.m_perms<<", memsz="<<seg->memsz<<", filesz="<<seg->filesz<<endl;

			segperms=perms;
			segstart=pagestart;
			segend=segstart+PAGE_SIZE;

			update_initend(perms);

		}
		else
		{
			// else, same permission and next page, extend segment. 
			segend=pagestart+PAGE_SIZE;
			if(! should_bss_optimize(perms) ) 
				initend=pagestart+PAGE_SIZE;
		}
		
	}

	// make sure we print the last one
	const auto seg=new LoadSegment_t(initend-segstart, segend-segstart, 0, segstart,segperms.m_perms);
	segvec.push_back(seg);

	cout<<"Found segment "<<hex<<segstart<<"-"<<(segend-1)<<", perms="<<segperms.m_perms<<", memsz="<<seg->memsz<<", filesz="<<seg->filesz<<endl;
	
}







template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr, class T_Elf_Shdr, class T_Elf_Sym, class T_Elf_Rel, class T_Elf_Rela, class T_Elf_Dyn>
void ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr, T_Elf_Shdr, T_Elf_Sym, T_Elf_Rel, T_Elf_Rela, T_Elf_Dyn>::LoadEhdr(FILE* fin) 
{
	fseek(fin,0,SEEK_SET);
	auto res=fread(&ehdr,sizeof(ehdr), 1, fin);
	assert(res==1);
	ehdr_to_host(ehdr);
};

template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr, class T_Elf_Shdr, class T_Elf_Sym, class T_Elf_Rel, class T_Elf_Rela, class T_Elf_Dyn>
void ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr,T_Elf_Shdr, T_Elf_Sym, T_Elf_Rel, T_Elf_Rela, T_Elf_Dyn>::LoadPhdrs(FILE* fin) 
{
	fseek(fin,ehdr.e_phoff,SEEK_SET);
	phdrs.resize(ehdr.e_phnum);
	for(unsigned int i=0;i<phdrs.size();i++)
	{
		auto res=fread(&phdrs[i], sizeof(phdrs[i]), 1, fin);
		assert(res==1);
		phdr_to_host(ehdr,phdrs[i]);
	}
};

template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr, class T_Elf_Shdr, class T_Elf_Sym, class T_Elf_Rel, class T_Elf_Rela, class T_Elf_Dyn>
void ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr,T_Elf_Shdr, T_Elf_Sym, T_Elf_Rel, T_Elf_Rela, T_Elf_Dyn>::CreateNewPhdrs(
	const IRDB_SDK::VirtualOffset_t &min_addr, const IRDB_SDK::VirtualOffset_t &max_addr) 
{
	
	if(CreateNewPhdrs_GapAllocate(min_addr, max_addr))
	{
		cout<<"ElfWriter Success with GapAllocate"<<endl;
		return;
	}
	else if(CreateNewPhdrs_FirstPageAllocate(min_addr, max_addr))
	{
		cout<<"ElfWriter Success with FirstPageAllocate"<<endl;
		return;
	}
	else if(CreateNewPhdrs_PreAllocate(min_addr, max_addr))
	{
		cout<<"ElfWriter Success with PreAllocate"<<endl;
		return;
	}
	else if(CreateNewPhdrs_PostAllocate(min_addr, max_addr))
	{
		cout<<"ElfWriter Success with PostAllocate"<<endl;
		return;
	}
	else
	{
		cout<<"ElfWriter cannot find a place in the program for the PHDRS."<<endl;
		assert(0);
	}
	
}

template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr, class T_Elf_Shdr, class T_Elf_Sym, class T_Elf_Rel, class T_Elf_Rela, class T_Elf_Dyn>
bool ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr,T_Elf_Shdr,T_Elf_Sym, T_Elf_Rel, T_Elf_Rela, T_Elf_Dyn>::CreateNewPhdrs_PostAllocate(
	const IRDB_SDK::VirtualOffset_t &min_addr, const IRDB_SDK::VirtualOffset_t &max_addr) 
{
	// post allocation not enabled, yet.
	return false;
}

template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr, class T_Elf_Shdr, class T_Elf_Sym, class T_Elf_Rel, class T_Elf_Rela, class T_Elf_Dyn>
bool ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr,T_Elf_Shdr,T_Elf_Sym, T_Elf_Rel, T_Elf_Rela, T_Elf_Dyn>::CreateNewPhdrs_FirstPageAllocate(
	const IRDB_SDK::VirtualOffset_t &min_addr, const IRDB_SDK::VirtualOffset_t &max_addr) 
{
	// check to see if there's room on the first page for 
	unsigned int phdr_size=DetermineMaxPhdrSize();
	if(page_align(min_addr)+sizeof(T_Elf_Ehdr)+phdr_size > min_addr)
		return false;
	// this is an uncommon case -- we are typically adding
	// segments and so the segment map won't fit on the first page.
	// if this assertion hits, email jdhiser@gmail.com and attach your input pgm,
	// then convert this to a return false to avoid assertion until he fixes it;
	assert(0);
}

template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr, class T_Elf_Shdr, class T_Elf_Sym, class T_Elf_Rel, class T_Elf_Rela, class T_Elf_Dyn>
bool ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr, T_Elf_Shdr,T_Elf_Sym, T_Elf_Rel, T_Elf_Rela, T_Elf_Dyn>::readonly_space_at(
	const IRDB_SDK::VirtualOffset_t addr, const unsigned int size)
{
	for(unsigned int i=0;i<size;i++)
	{
		IRDB_SDK::VirtualOffset_t page=page_align(addr+i);
		IRDB_SDK::VirtualOffset_t page_offset=addr+i-page;
	
		// page not allocated yet, go ahead and call this byte free.
		if(pagemap.find(page) == pagemap.end())
			continue;

		if(pagemap.at(page).inuse[page_offset])
			return false;

		// check that the page is not writable.  4=r, 2=w, 1=x.
		if((pagemap.at(page).m_perms & 0x2) != 0)
			return false;
	}
	return true;
}

template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr, class T_Elf_Shdr, class T_Elf_Sym, class T_Elf_Rel, class T_Elf_Rela, class T_Elf_Dyn>
int ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr,T_Elf_Shdr,T_Elf_Sym, T_Elf_Rel, T_Elf_Rela, T_Elf_Dyn>::locate_segment_index(const IRDB_SDK::VirtualOffset_t addr)
{
	// segment's are sorted by address.
	for(unsigned int i=0;i<segvec.size();i++)
	{
		// if the filesz diverges from the memsz, then we have an issue.
		// luckily, this only happens for bss, and there's almost always space
		// after the text for the new phdr.
		if(segvec[i]->filesz < segvec[i]->memsz)
			return -1; 

		// if the new_phdr_addr is in this segment.
		if(segvec[i]->start_page <= addr  && addr < segvec[i]->start_page + segvec[i]->filesz)
		{
			return i;
		}
	}
	return -1;
}

template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr, class T_Elf_Shdr, class T_Elf_Sym, class T_Elf_Rel, class T_Elf_Rela, class T_Elf_Dyn>
unsigned int ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr,T_Elf_Shdr,T_Elf_Sym, T_Elf_Rel, T_Elf_Rela, T_Elf_Dyn>::count_filesz_to_seg(unsigned int seg)
{
	unsigned int filesz=0;
	// segment's are sorted by address.
	for(unsigned int i=0;i<seg;i++)
	{
		filesz+=segvec[i]->filesz;
	}
	return filesz;
}

template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr, class T_Elf_Shdr, class T_Elf_Sym, class T_Elf_Rel, class T_Elf_Rela, class T_Elf_Dyn>
bool ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr,T_Elf_Shdr,T_Elf_Sym, T_Elf_Rel, T_Elf_Rela, T_Elf_Dyn>::CreateNewPhdrs_GapAllocate(
	const IRDB_SDK::VirtualOffset_t &min_addr, const IRDB_SDK::VirtualOffset_t &max_addr) 
{
	/* for shared objects, we need the PHDR file offset to be equal to the  
	 * memory offset because the kernel passes base_address+Ehdr::ph_off via
	 * the auxv array to ld.so.  Where ld.so then uses that as an address.
	 */

	// gap allocate assumes there's space on the first page for the EHdrs.  If there's not, 
	// try pre-allocating.
	if(page_align(min_addr)+sizeof(T_Elf_Ehdr) >= min_addr)
		return false;

	// first, find the first free space that's big enough.
	unsigned int phdr_size=DetermineMaxPhdrSize();
	IRDB_SDK::VirtualOffset_t new_phdr_addr=0;
	for(unsigned int i=min_addr;i<max_addr; i++)
	{
		if(readonly_space_at(i,phdr_size))
		{
			new_phdr_addr=i;
			break;
		}
	
	}

	// find segment
	int new_phdr_segment_index=locate_segment_index(new_phdr_addr-1);

	// if there's no segment for the start, we'll have to allocate a page anyhow.  just use the _Preallocate routine.
	if(new_phdr_segment_index==-1)
		return false;


	// we've seen multi-page phdrs with NogOF (non-overlapping globals with overflow protection)
	// and PreAllocate fails on PIE exe's and .so's, so let's try a bit harder to GapAllocate.
	// a multiple-page phdr can't be gap allocated.
	//if(phdr_size>=PAGE_SIZE)
	//	return false;

	// verify that the segment can be extended.
	int pages_to_extend=0;	// extend the segment by a page.
	for(unsigned int i=0;i<phdr_size; i++)
	{
		IRDB_SDK::VirtualOffset_t this_addr=new_phdr_addr+i;
		IRDB_SDK::VirtualOffset_t this_page=page_align(this_addr);
		// find segment for phdr+i
		int seg=locate_segment_index(this_addr);

		// if it's not allocated, we ran off the end of the segment.  
		// if we're also at the first byte of a page, extend the segment by a page.
		if(seg==-1 && this_page==this_addr )
			pages_to_extend++;

		// this should be safe because the new page can't be in a segment already.
		if(seg==-1)
			continue;

		if(seg == new_phdr_segment_index)
			continue;
		// uh oh, we found that the phdr would cross into the next segment 
		// but, we know there's enough space in the next segment because 
		// 	1) read_only_space_at returned true, so there's enough free space.
		// 	2) the free space cannot transcend the entire next segment, because that would mean the entire 
		//	   segment was empty (implying the segment is redundant).  
		// Thus, we can just stop looking here and extend the previous segment to abut the new segment. 
		// and the phdrs will span the two segments.
		for(auto j=i+1;j<phdr_size; j++)
			assert(seg==locate_segment_index(new_phdr_addr+j));
		break;
	}

	// if we get here, we've found a spot for the PHDR.

	// mark the bytes of the pages as readable, and in-use.
	for(unsigned int i=0;i<phdr_size; i++)
	{
		IRDB_SDK::VirtualOffset_t this_addr=new_phdr_addr+i;
		IRDB_SDK::VirtualOffset_t this_page=page_align(this_addr);
		IRDB_SDK::VirtualOffset_t this_offset=this_addr-this_page;
		pagemap[this_page].inuse[this_offset]=true;
		pagemap[this_page].union_permissions(0x4); // add read permission
	}
	segvec[new_phdr_segment_index]->filesz+=(PAGE_SIZE*pages_to_extend);
	segvec[new_phdr_segment_index]->memsz+=(PAGE_SIZE*pages_to_extend);

	unsigned int fileoff=count_filesz_to_seg(new_phdr_segment_index);
	fileoff+=(new_phdr_addr-segvec[new_phdr_segment_index]->start_page);

	return CreateNewPhdrs_internal(min_addr,max_addr,0x0,false,fileoff, new_phdr_addr);

}

template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr, class T_Elf_Shdr, class T_Elf_Sym, class T_Elf_Rel, class T_Elf_Rela, class T_Elf_Dyn>
bool ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr,T_Elf_Shdr,T_Elf_Sym, T_Elf_Rel, T_Elf_Rela, T_Elf_Dyn>::CreateNewPhdrs_PreAllocate(
	const IRDB_SDK::VirtualOffset_t &min_addr, const IRDB_SDK::VirtualOffset_t &max_addr) 
{
	auto phdr_size=DetermineMaxPhdrSize();
	auto aligned_phdr_size=page_round_up(phdr_size);
	auto total_header_size=phdr_size+sizeof(T_Elf_Ehdr);
	//auto aligned_min_addr=page_align(min_addr);


	/* check to see if it will fit in the address space above the first pinned address */
	if(total_header_size > min_addr)
		return false;

	IRDB_SDK::VirtualOffset_t new_phdr_addr=(T_Elf_Addr)page_align(min_addr)-PAGE_SIZE+sizeof(T_Elf_Ehdr);
	return CreateNewPhdrs_internal(min_addr,max_addr,aligned_phdr_size,true, sizeof(T_Elf_Ehdr), new_phdr_addr);
}

template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr, class T_Elf_Shdr, class T_Elf_Sym, class T_Elf_Rel, class T_Elf_Rela, class T_Elf_Dyn>
DataScoop_t* ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr,T_Elf_Shdr,T_Elf_Sym, T_Elf_Rel, T_Elf_Rela, T_Elf_Dyn>::find_scoop_by_name(const string& name, FileIR_t* firp)
{
	for(DataScoopSet_t::iterator it=firp->getDataScoops().begin(); it!=firp->getDataScoops().end(); ++it)
	{
		DataScoop_t* scoop=*it;
		if(scoop->getName()==name)
			return scoop;
	}

	return nullptr;
}

template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr, class T_Elf_Shdr, class T_Elf_Sym, class T_Elf_Rel, class T_Elf_Rela, class T_Elf_Dyn>
void  ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr,T_Elf_Shdr,T_Elf_Sym, T_Elf_Rel, T_Elf_Rela, T_Elf_Dyn>::update_phdr_for_scoop_sections(FileIR_t* firp)
{
	// look at each header.
	for(unsigned i=0;i<new_phdrs.size(); i++)
	{

		// this struct is a table/constant for mapping PT_names to section names.
		struct pt_type_to_sec_name_t
		{
			unsigned int pt_type;
			const char* sec_name;
		}	pt_type_to_sec_name[] = 
		{
			{PT_INTERP, ".interp"},
			{PT_DYNAMIC, ".dynamic"},
			{PT_NOTE, ".note.ABI-tag"},
			{PT_GNU_EH_FRAME, ".eh_frame_hdr"}
		};

		// check if a type of header listed above.
		for(unsigned k=0;k<(sizeof(pt_type_to_sec_name)/sizeof(pt_type_to_sec_name_t)); k++)
		{
			// check if a type of header listed above.
			if(new_phdrs[i].p_type==pt_type_to_sec_name[k].pt_type)
			{
				// grab the name from the const table..
				// and find the scoop
				DataScoop_t* scoop=find_scoop_by_name(pt_type_to_sec_name[k].sec_name, firp);

				if(scoop)
				{
					new_phdrs[i].p_vaddr=scoop->getStart()->getVirtualOffset();
					new_phdrs[i].p_paddr=scoop->getStart()->getVirtualOffset();
					new_phdrs[i].p_filesz= scoop->getEnd()->getVirtualOffset() - scoop->getStart()->getVirtualOffset() + 1;
					new_phdrs[i].p_memsz = scoop->getEnd()->getVirtualOffset() - scoop->getStart()->getVirtualOffset() + 1;

					new_phdrs[i].p_offset=0;
					for(unsigned j=0;j<new_phdrs.size(); j++)
					{
						if( new_phdrs[j].p_vaddr<= new_phdrs[i].p_vaddr && 
						    new_phdrs[i].p_vaddr < new_phdrs[j].p_vaddr+new_phdrs[j].p_filesz)
						{
							new_phdrs[i].p_offset=new_phdrs[j].p_offset + new_phdrs[i].p_vaddr - new_phdrs[j].p_vaddr;
						}
					}
					assert(new_phdrs[i].p_offset!=0);
				}
			}	
		}
	}
	return;
}
template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr, class T_Elf_Shdr, class T_Elf_Sym, class T_Elf_Rel, class T_Elf_Rela, class T_Elf_Dyn>
void ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr,T_Elf_Shdr,T_Elf_Sym, T_Elf_Rel, T_Elf_Rela, T_Elf_Dyn>::trim_last_segment_filesz(FileIR_t* firp)
{
	// this seems OK and is necessary on centos... but on ubuntu, it's a no-go (segfault from loader).
	// for now, disabling this optimization until we can figure out what's up.
	
#if 0
	// skip trimming if we aren't doing bss optimization.
	if(!m_bss_opts) return;


	// find the last pt load segment header.
	auto seg_to_trim=-1;
	for(auto i=0u;i<new_phdrs.size(); i++)
	{
		if(new_phdrs[i].p_type==PT_LOAD)
		{
			seg_to_trim=i;
		}
		
	}
	assert(seg_to_trim!=-1);


	

	const auto seg_start_addr=new_phdrs[seg_to_trim].p_vaddr;
	const auto seg_filesz=new_phdrs[seg_to_trim].p_filesz;
	const auto seg_end_addr=seg_start_addr+seg_filesz-1;
	const auto seg_end_page_start=page_round_down(seg_end_addr);
	const auto &page=pagemap[seg_end_page_start];

	// don't write 0's at end of last page 
	auto k=PAGE_SIZE-1;
	for (/* above */; k>=0; k--)
	{
		if(page.data[k]!=0)
			break;
	}
	const auto last_nonzero_byte_in_seg = k;
	(void)last_nonzero_byte_in_seg;

	const auto bytes_to_trim = 1 (PAGE_SIZE - 1) - last_nonzero_byte_in_seg;	

	// lastly, update the filesz so we don't need the reste of those bytes.
	// memsz is allowed to be bigger than filesz.
 	new_phdrs[seg_to_trim].p_filesz -= (bytes_to_trim);

	return;
#endif

}




template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr, class T_Elf_Shdr, class T_Elf_Sym, class T_Elf_Rel, class T_Elf_Rela, class T_Elf_Dyn>
bool ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr,T_Elf_Shdr,T_Elf_Sym, T_Elf_Rel, T_Elf_Rela, T_Elf_Dyn>::CreateNewPhdrs_internal(
	const IRDB_SDK::VirtualOffset_t &min_addr, 
	const IRDB_SDK::VirtualOffset_t &max_addr,
	const int &first_seg_file_offset,
	const bool &add_pt_load_for_phdr,
	const size_t phdr_map_offset,
	IRDB_SDK::VirtualOffset_t new_phdr_addr
	) 
{
	

	std::cout<<"Assigning phdr to address "<<std::hex<<new_phdr_addr<<std::endl;
	std::cout<<"Assigning first seg to a file offset that's at least: "<<std::hex<<first_seg_file_offset<<std::endl;

	// create a load segments into the new header list.
	// assume hdr are on first page.
	unsigned int fileoff=first_seg_file_offset;
	for(unsigned int i=0;i<segvec.size();i++)
	{
		T_Elf_Phdr thisphdr;
		memset(&thisphdr,0,sizeof(thisphdr));
		thisphdr.p_type = PT_LOAD; 
		thisphdr.p_flags = (ELFIO::Elf_Word)segvec[i]->m_perms;      
		thisphdr.p_offset = fileoff;     
		std::cout<<"Assigning load["<<dec<<i<<"].ph_offset="<<std::hex<<fileoff<<std::endl;
		thisphdr.p_vaddr = (T_Elf_Addr)segvec[i]->start_page; 
		thisphdr.p_paddr = (T_Elf_Addr)segvec[i]->start_page; 
		thisphdr.p_filesz = (ELFIO::Elf_Xword)segvec[i]->filesz; 
		thisphdr.p_memsz = (ELFIO::Elf_Xword)segvec[i]->memsz; 
		thisphdr.p_align=  0x1000;

		new_phdrs.push_back(thisphdr);

		// advance file pointer.
		fileoff+=segvec[i]->filesz;

	}


	// go through orig. phdrs any copy and that aren't of a type we are re-createing.
	for(unsigned int i=0;i<phdrs.size();i++)
	{
		// skip any load headers, the irdb tells us what to load.
		if(phdrs[i].p_type == PT_LOAD)
			continue;

		// skip phdr header.
		if(phdrs[i].p_type == PT_PHDR)
			continue;

		// skip RELRO header, we're relocating stuff and wil have to create 1 or more.
		if(phdrs[i].p_type == PT_GNU_RELRO)
			continue;
		
		T_Elf_Phdr newphdr=phdrs[i];

// figure out how to make this an xform/step in $PS.
// instead of always doing it.
#if 0
		if(phdrs[i].p_type == PT_GNU_STACK)
			newphdr.p_flags &= ~PF_X; // turn off executable stack.
#endif

		// find offset in loadable segment
		// using segvec.size() instead of new_phdrs size to search for segments in the new list.
		for(unsigned int j=0;j<segvec.size();j++)
		{
			if(new_phdrs[j].p_vaddr <= newphdr.p_vaddr  && newphdr.p_vaddr <= new_phdrs[j].p_vaddr+new_phdrs[j].p_filesz)
			{
				newphdr.p_offset=new_phdrs[j].p_offset+(newphdr.p_vaddr - new_phdrs[j].p_vaddr);
				break;
			}
		}

		// update offset
		new_phdrs.push_back(newphdr);
	}

	if(add_pt_load_for_phdr)
	{
		const T_Elf_Addr min_phdr_size=(new_phdrs.size()+1 ) * sizeof(T_Elf_Phdr);
		const T_Elf_Addr phdr_size=page_round_up(min_phdr_size);

		// specify a load section for the new program's header and phdr
		T_Elf_Phdr newheaderphdr;
		memset(&newheaderphdr,0,sizeof(newheaderphdr));
		newheaderphdr.p_type = PT_LOAD;
		newheaderphdr.p_flags =(ELFIO::Elf_Word)4;
		newheaderphdr.p_offset =0;
		newheaderphdr.p_vaddr =(T_Elf_Addr)page_align(new_phdr_addr);
		newheaderphdr.p_paddr =(T_Elf_Addr)page_align(new_phdr_addr);
		newheaderphdr.p_filesz =(ELFIO::Elf_Xword)phdr_size;
		newheaderphdr.p_memsz =(ELFIO::Elf_Xword)phdr_size;
		newheaderphdr.p_align =0x1000;
		new_phdrs.insert(new_phdrs.begin(),newheaderphdr);

		auto size=newheaderphdr.p_vaddr+newheaderphdr.p_memsz; 
		auto start_addr=newheaderphdr.p_vaddr;
		for(VirtualOffset_t i=page_align(newheaderphdr.p_vaddr); i<newheaderphdr.p_vaddr+newheaderphdr.p_memsz; i+=PAGE_SIZE)
		{
			cout<<"Updating pagemap for new phdr. To page: "<<hex<<i<<", perms="<<newheaderphdr.p_flags
			    << " start="<<hex<< newheaderphdr.p_vaddr
			    << " end="<<hex<< size << endl;
			pagemap[i].union_permissions(newheaderphdr.p_vaddr+newheaderphdr.p_memsz);
			pagemap[i].is_relro |= false;
			for(int j=0;j<PAGE_SIZE;j++)
			{
				// get the data out of the scoop and put it into the page map.
				if(start_addr <= i+j && i+j < start_addr + size)
				{
					pagemap[i].data[j]=0xf4; // don't update, phdrs are written separately.  we just need space 
								 // in the map.
					pagemap[i].inuse[j]=true;
				}
			}
		}

	}

	// create the new phdr phdr
	std::cout<<"New phdrs at: "<<std::hex<<new_phdr_addr<<std::endl;
	T_Elf_Phdr newphdrphdr;
	memset(&newphdrphdr,0,sizeof(newphdrphdr));
	newphdrphdr.p_type = PT_PHDR;
	newphdrphdr.p_flags =(ELFIO::Elf_Word)4;
	newphdrphdr.p_offset =phdr_map_offset;
	newphdrphdr.p_vaddr = new_phdr_addr;
	newphdrphdr.p_paddr = new_phdr_addr;
	newphdrphdr.p_filesz =(ELFIO::Elf_Xword)(new_phdrs.size()+1)*getSegHeaderSize();
	newphdrphdr.p_memsz =(ELFIO::Elf_Xword)(new_phdrs.size()+1)*getSegHeaderSize();
	newphdrphdr.p_align =0x1000;
	new_phdrs.insert(new_phdrs.begin(),newphdrphdr);


	std::vector<T_Elf_Phdr> relro_phdrs;
	new_phdrs.insert(new_phdrs.end(), relro_phdrs.begin(), relro_phdrs.end());

	// record the new ehdr.
	new_ehdr=ehdr;
	new_ehdr.e_phoff=phdr_map_offset;
	new_ehdr.e_shoff=0;
	new_ehdr.e_shnum=0;
	new_ehdr.e_phnum=new_phdrs.size();
	new_ehdr.e_shstrndx=0;

	update_phdr_for_scoop_sections(m_firp);
	trim_last_segment_filesz(m_firp);

	return true;
}

template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr, class T_Elf_Shdr, class T_Elf_Sym, class T_Elf_Rel, class T_Elf_Rela, class T_Elf_Dyn>
void ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr,T_Elf_Shdr,T_Elf_Sym, T_Elf_Rel, T_Elf_Rela, T_Elf_Dyn>::WriteElf(FILE* fout)
{
	assert(fout);

	// write the segments first, as they may overlap the ehdr and phdrs
	// and we maintain those separately.

	// look at enach phdr entry, and write data as it says to.
	unsigned int loadcnt=0;
	for(unsigned int i=0;i<new_phdrs.size();i++)
	{
		if(new_phdrs[i].p_type==PT_LOAD)
		{
			loadcnt++;
		
			std::cout<<"phdr["<<std::dec<<loadcnt<<"] writing at:"<<std::hex<<new_phdrs[i].p_offset<<std::endl;
			fseek(fout,new_phdrs[i].p_offset,SEEK_SET);
			for(unsigned int j=0;j<new_phdrs[i].p_filesz;j+=PAGE_SIZE)
			{
				const PageData_t &page=pagemap[new_phdrs[i].p_vaddr+j];

				// is it the last page?
				if(j+PAGE_SIZE < new_phdrs[i].p_filesz || m_write_sections )
				{
					fwrite(page.data.data(), PAGE_SIZE, 1, fout);
				}
				else
				{
#if 0
note:  this does not work on centos as it can leave the segments
start address + filesize to be a number greater than the entire files size.
This causes the loader to complain.  The "right" way to do this is to update the
filesz before we start writing out the elf.  See "trim_last_seg_filesz"
					// don't write 0's at end of last page 
					int k=0;
					for (k=PAGE_SIZE-1;k>=0;k--)
					{
						if(page.data[k]!=0)
							break;
					}
					std::cout<<"phdr["<<std::dec<<loadcnt<<"] optimizing last page write to size k="<<std::hex<<k<<std::endl;
#endif
					const auto end_offset=new_phdrs[i].p_filesz;
					const auto current_offset = j;
					const auto bytes_to_write = end_offset - current_offset;
					assert(bytes_to_write <= PAGE_SIZE);
					fwrite(page.data.data(), bytes_to_write, 1, fout);
				}
		
			}
		}
	}

	// write the header.
	fseek(fout,0,SEEK_SET);
	auto new_ehdr_endian_correct = new_ehdr;
	host_to_ehdr(new_ehdr_endian_correct);
	fwrite(&new_ehdr_endian_correct, sizeof(new_ehdr), 1, fout);
	// write the phdrs, which may be part of a segment written above.
	cout << "Writing segment headers at " << hex << new_ehdr.e_phoff << ", size=" << new_phdrs.size()*sizeof(new_phdrs[0]) << endl;
	fseek(fout,new_ehdr.e_phoff,SEEK_SET);

	// doesn't account for endian issues
//	fwrite(new_phdrs.data(), sizeof(new_phdrs[0]), new_phdrs.size(), fout);
	for(auto phdr : new_phdrs)
	{
		host_to_phdr(ehdr,phdr);
		fwrite(&phdr, sizeof(new_phdrs[0]), 1, fout);

	}
}

template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr, class T_Elf_Shdr, class T_Elf_Sym, class T_Elf_Rel, class T_Elf_Rela, class T_Elf_Dyn>
unsigned int ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr,T_Elf_Shdr,T_Elf_Sym, T_Elf_Rel, T_Elf_Rela, T_Elf_Dyn>::DetermineMaxPhdrSize()
{
	unsigned int phdr_count=0;
	/* count phdr's that aren't pt_load or pt_phdr */
	for(unsigned int i=0;i<phdrs.size();i++)
	{
		// skip any load sections, the irdb tells us what to load.
		if(phdrs[i].p_type == PT_LOAD)
			continue;

		// skip phdr section.
		if(phdrs[i].p_type == PT_PHDR)
			continue;
		
		phdr_count++;
	}

	// add a phdr for each segment that needs a mapping
	phdr_count+=segvec.size()*2; 	// each entry in the segvec may need a relro header.
	
	// add 2 more sections that 1) of type PT_PHDR, and 2) a possible PT_LOAD section to load the phdr.
	// worst case is we allocate an entire new page for it.
	phdr_count+=2;		

	return phdr_count*sizeof(T_Elf_Phdr);
}

template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr, class T_Elf_Shdr, class T_Elf_Sym, class T_Elf_Rel, class T_Elf_Rela, class T_Elf_Dyn>
void ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr,T_Elf_Shdr,T_Elf_Sym, T_Elf_Rel, T_Elf_Rela, T_Elf_Dyn>::AddSections(FILE* fout)
{
	fseek(fout,0,SEEK_END);
	long cur_file_pos=ftell(fout);


	StringTable_t strtab;
	map<DataScoop_t*,size_t> file_positions;
	vector<T_Elf_Shdr> shdrs;

	// add each scoop name to the string table
	// for_each(m_firp->getDataScoops().begin(), m_firp->getDataScoops().end(), [&](DataScoop_t* scoop)
	for(auto scoop : m_firp->getDataScoops()) 
	{
		strtab.AddString(scoop->getName());
	};


	string zipr_symtab=".scoop_symtab";
	strtab.AddString(zipr_symtab);
	string null_symtab="nullptr";
	strtab.AddString(null_symtab);


	// locate a file offset for each scoop by examining the output phdrs.
	// for_each(m_firp->getDataScoops().begin(), m_firp->getDataScoops().end(), [&](DataScoop_t* scoop)
	for(auto scoop : m_firp->getDataScoops()) 
	{
		auto finder=find_if(new_phdrs.begin(), new_phdrs.end(), [scoop](const T_Elf_Phdr& phdr)
		{
			return (phdr.p_vaddr <= scoop->getStart()->getVirtualOffset() && 
				scoop->getStart()->getVirtualOffset() < phdr.p_vaddr+phdr.p_memsz);
	 	});
		// assert we found it.
		assert(finder!=new_phdrs.end());
		const T_Elf_Phdr& phdr=*finder;

		size_t filepos=phdr.p_offset + (scoop->getStart()->getVirtualOffset()-phdr.p_vaddr);
		file_positions[scoop]=filepos;
	};

	T_Elf_Shdr null_shdr;
	memset(&null_shdr,0,sizeof(null_shdr));
	null_shdr.sh_type=SHT_NULL;
	null_shdr. sh_name =strtab.location(null_symtab);

	shdrs.push_back(null_shdr);

	struct section_type_map_t
	{	
		string name;
		unsigned int type;
		unsigned int sh_ent_size;
		string link;
	} section_type_map[]={
		{".init_array",   SHT_INIT_ARRAY,	0, 			"" },
		{".fini_array",   SHT_FINI_ARRAY,	0, 			"" },
		{".dynamic", 	  SHT_DYNAMIC,		sizeof(T_Elf_Dyn),	".dynstr"},
		{".note.ABI-tag", SHT_NOTE,		0,  			""},
		{".note.gnu.build-id", SHT_NOTE,	0,  			""},
		{".gnu.hash",     SHT_GNU_HASH,		0,  			".dynsym"},
		{".dynsym",       SHT_DYNSYM, 		sizeof(T_Elf_Sym),  	".dynstr"},
		{".dynstr",       SHT_STRTAB, 		0, 		  	""},
		{".shstrtab",     SHT_STRTAB, 		0, 		  	""},
		{".symtab",       SHT_SYMTAB, 		sizeof(T_Elf_Sym),  	""},
		{".strtab",       SHT_STRTAB, 		0, 		  	""},
		{".rel.dyn",      SHT_REL,    		sizeof(T_Elf_Rel),  	""},
		{".rela.dyn",     SHT_RELA,   		sizeof(T_Elf_Rela), 	".dynsym"},
		{".rel.plt",      SHT_REL,    		sizeof(T_Elf_Rel),  	".dynsym"},
		{".rela.plt",     SHT_RELA,   		sizeof(T_Elf_Rela), 	".dynsym"},
		{".gnu.version",  SHT_GNU_versym,    	2, 			".dynsym"},
		{".gnu.version_r",SHT_GNU_verneed,    	0,		  	".dynstr"},
		{".rela.dyn coalesced w/.rela.plt",     SHT_RELA,   		sizeof(T_Elf_Rela), 	".dynsym"}
	};


	// for each scoop, pushback an shdr
	// for_each(m_firp->getDataScoops().begin(), m_firp->getDataScoops().end(), [&](DataScoop_t* scoop)
	for(auto scoop : m_firp->getDataScoops())
	{

		T_Elf_Shdr shdr;
		shdr. sh_name =strtab.location(scoop->getName());

		auto it=find_if(begin(section_type_map), end(section_type_map), [&scoop](const section_type_map_t &sm)	
				{
					return scoop->getName()==sm.name;
				});
		if(end(section_type_map) != it)
		{
			cout<<"Setting ent-size for "<<scoop->getName()<<" to "<<dec<<it->sh_ent_size<<endl;
			shdr. sh_type = it->type;	// sht_progbits, sht, sht_strtab, sht_symtab, ...
			shdr. sh_entsize = it->sh_ent_size;	
		}
		else
		{
			shdr. sh_type = SHT_PROGBITS;	// sht_progbits, sht, sht_strtab, sht_symtab, ...
			shdr. sh_entsize = 0;
		}
		shdr. sh_flags = SHF_ALLOC; // scoop->getRawPerms();
		if(scoop->isExecuteable())
			shdr. sh_flags |= SHF_EXECINSTR; 
		if(scoop->isWriteable())
			shdr. sh_flags |= SHF_WRITE; 
		shdr. sh_addr = scoop->getStart()->getVirtualOffset();
		shdr. sh_offset =file_positions[scoop];
		shdr. sh_size = scoop->getEnd()->getVirtualOffset() - scoop->getStart()->getVirtualOffset() + 1;
		shdr. sh_link = SHN_UNDEF;	
		shdr. sh_info = 0 ;
		shdr. sh_addralign= 0 ; // scoop->getAlign(); doesn't exist?
	
		shdrs.push_back(shdr);
	};
	auto scoop_it=m_firp->getDataScoops().begin();
	for(unsigned int i=1; i<shdrs.size(); i++)	 // skip null shdr
	{
 		T_Elf_Shdr & shdr = shdrs[i];
		auto map_it=find_if(begin(section_type_map), end(section_type_map), [&scoop_it](const section_type_map_t &sm)	
			{
				return (*scoop_it)->getName()==sm.name;
			});
		if(end(section_type_map) != map_it && map_it->link!="")
		{
			auto link_it=m_firp->getDataScoops().begin();
			for(unsigned int j=1; j<shdrs.size(); j++) // skip null shdr
			{
				if((*link_it)->getName() == map_it->link)
				{
					shdr.sh_link=j;
					break;
				}
				link_it++;
			}
		}
		scoop_it++;
	}

	T_Elf_Shdr symtab_shdr;
	symtab_shdr. sh_name =strtab.location(zipr_symtab);
	symtab_shdr. sh_type = SHT_STRTAB;	
	symtab_shdr. sh_flags = 0;
	symtab_shdr. sh_addr = 0;
	symtab_shdr. sh_offset = cur_file_pos;
	symtab_shdr. sh_size = strtab.size();
	symtab_shdr. sh_link = SHN_UNDEF;	
	symtab_shdr. sh_info = 0;
	symtab_shdr. sh_addralign=0;
	symtab_shdr. sh_entsize =0;
	shdrs.push_back(symtab_shdr);

	cout<<"Writing strtab at filepos="<<hex<<cur_file_pos<<endl;
	strtab.Write(fout);

	long shdr_file_pos=ftell(fout);
	
	cout<<"Writing section headers at filepos="<<hex<<shdr_file_pos<<endl;
	// doesn't account for endianness
//	fwrite(shdrs.data(), sizeof(T_Elf_Shdr), shdrs.size(), fout);
	for(auto shdr : shdrs)
	{
		host_to_shdr(ehdr,shdr);
		fwrite(&shdrs, sizeof(T_Elf_Shdr), 1, fout);

	}

	new_ehdr.e_shentsize=sizeof(T_Elf_Shdr);
	new_ehdr.e_shnum=shdrs.size();
	new_ehdr.e_shstrndx=shdrs.size()-1;	// symtab was added last.
	new_ehdr.e_shoff=shdr_file_pos;

	// rewrite the file header so that sections are listed.
	fseek(fout,0,SEEK_SET);
	auto endian_ehdr = new_ehdr;
	host_to_ehdr(endian_ehdr);
	fwrite(&endian_ehdr, sizeof(new_ehdr),1,fout);
}

//  explicit instantation of methods for 32- and 64-bit classes.
template class ElfWriterImpl<ELFIO::Elf64_Ehdr, ELFIO::Elf64_Phdr, ELFIO::Elf64_Addr, ELFIO::Elf64_Shdr, ELFIO::Elf64_Sym, ELFIO::Elf64_Rel, ELFIO::Elf64_Rela, ELFIO::Elf64_Dyn>;
template class ElfWriterImpl<ELFIO::Elf32_Ehdr, ELFIO::Elf32_Phdr, ELFIO::Elf32_Addr, ELFIO::Elf32_Shdr, ELFIO::Elf32_Sym, ELFIO::Elf32_Rel, ELFIO::Elf32_Rela, ELFIO::Elf32_Dyn>;

