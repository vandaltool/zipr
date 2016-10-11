
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <link.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define strcmp Zest_Strcmp
#define strlen Zest_Strlen

// typedef for a function pointer because the syntax is otherwise insane, unreadable and unwriteable.
typedef void (*zestcfi_func_ptr_t)(ElfW(Addr)) ;

#define DEBUG 0


static int strcmp(const char *s1, const char *s2)
{
	for ( ; *s1 == *s2; s1++, s2++)
		if (*s1 == '\0')
			return 0;
	return ((*(unsigned char *)s1 < *(unsigned char *)s2) ? -1 : +1);
}

static size_t strlen( const char *str )
{
	register const char *s;
	for (s = str; *s; ++s);
	return(s - str);
}

static void write_str(const char* out)
{
#if DEBUG
	write(2,out,strlen(out));
#endif
}

static void write_ptr(const void* out)
{
#if DEBUG
	char buf[20];	
	sprintf(buf,"%p", out);
	write_str(buf);
#endif
}

static int ptload_for_target_exists(const struct dl_phdr_info *info, 
	const ElfW(Phdr) *phdr_start, const int phnum, const ElfW(Addr) target)
{
	const char* objname=info->dlpi_name;
	int i;
	for(i=0;i<phnum;i++)
	{
		// if it's a ptload header and it's
		if(phdr_start[i].p_type==PT_LOAD && (info->dlpi_addr + phdr_start[i].p_vaddr)<=target && 
			target < (info->dlpi_addr + phdr_start[i].p_vaddr+phdr_start[i].p_memsz) )
		{
			char str[]="Found target library: ";
			write_str(str);
			write_str(objname);
			write_str(".  ");
			return 1;	
		}
	}
	return 0;
}

static ElfW(Addr) find_dynamic_start(const struct dl_phdr_info *info,const ElfW(Phdr) *phdr_start, const int phnum)
{
	int i;
	for(i=0;i<phnum;i++)
	{
		if(phdr_start[i].p_type==PT_DYNAMIC)
			return info->dlpi_addr+phdr_start[i].p_vaddr;
	}

	return (ElfW(Addr))NULL;
}

static ElfW(Addr) find_dynamic_entry(const struct dl_phdr_info *info, const int type, const ElfW(Dyn) *dynamic_start)
{
	int i;
	for(i=0; dynamic_start[i].d_tag!=DT_NULL; i++)
	{
		if(dynamic_start[i].d_tag==type)
			return dynamic_start[i].d_un.d_val;
	}
	return 0;
}

static ElfW(Addr) find_symtab_entry(const char* tofind, const ElfW(Sym) *symtab, const char* strtab, const int strtabsz)
{
	int i;
	// unfortunately we can't know the symbol table size.
	// (really?!  i don't believe this, but I can't find it...)
	// but we do know the string table size.  
	// if a symbol's name points outside the string table, abort lookup as 
	// we've probably exceeded the symbol table's size.
	for(i=1; 1 ; i++)
	{
		// note:  st_name is ElfW(Word) which maps to uint32_t or uint64_t depending on architecture.
		// thus, no lower bound check is necessary bcause it has to be >=0.
		if(symtab[i].st_name>=strtabsz)
			return 0;

		const char* symstring=strtab+symtab[i].st_name;
#if DEBUG
		write_str("Checking symbol: ");
		write_str(symstring);
		write_str("\n");
#endif
		if(strcmp(symstring, tofind)==0)
			return (ElfW(Addr))&symtab[i];
	}

	// should not be reachable.  do not put assert(0) here because that calls libc, which is disallowed.
	return 0;
}

int dl_iterate_phdr_callback (struct dl_phdr_info *info, size_t size, void *data)
{
	ElfW(Addr) *zestcfi_addr=(ElfW(Addr)*)data;
	ElfW(Addr) target=*zestcfi_addr;

	const char* objname=info->dlpi_name;
	const ElfW(Phdr) *phdr_start=info->dlpi_phdr;
	const ElfW(Half) phnum=info->dlpi_phnum;

	// return now if this object doesn't have a ptload seg for the target.
	if(!ptload_for_target_exists(info,phdr_start, phnum, target))
		return 0;

	const ElfW(Dyn) *dynamic_start=(ElfW(Dyn)*)find_dynamic_start(info, phdr_start, phnum);
	if(!dynamic_start) 
	{
		write_str("Couldn't find dynamic start. ");
		return 0;
	}

	const ElfW(Sym) *symtab=(ElfW(Sym)*)find_dynamic_entry(info,DT_SYMTAB, dynamic_start);
	// apparently some modules don't have a reloc for their symtab pointer properly entry.
	if(!ptload_for_target_exists(info, phdr_start, phnum, (ElfW(Addr))symtab))
		symtab=(ElfW(Addr))symtab+(ElfW(Addr))info->dlpi_addr;

	if(!symtab) 
	{
		write_str("Couldn't find symtab start. ");
		return 0;
	}

	const char* strtab=(const char*)find_dynamic_entry(info,DT_STRTAB, dynamic_start);
	// apparently some modules don't have a reloc for their symtab pointer properly entry.
	if(!ptload_for_target_exists(info, phdr_start, phnum, (ElfW(Addr))strtab))
		strtab=(ElfW(Addr))strtab+(ElfW(Addr))info->dlpi_addr;
	if(!strtab) 
	{
		write_str("Couldn't find strtab start. ");
		return 0;
	}

	const int strtabsz=(const int)find_dynamic_entry(info,DT_STRSZ, dynamic_start);
	if(strtabsz==0) 
	{
		write_str("Couldn't find strtab start. ");
		return 0;
	}

	const ElfW(Sym) *sym=(ElfW(Sym)*)find_symtab_entry("zestcfi", symtab, strtab, strtabsz);
	if(!sym) 
	{
		write_str("Couldn't find zestcfi symbol start. ");
		return 0;
	}
	
	write_str( "Found zestcfi string.");

	*zestcfi_addr=info->dlpi_addr+sym->st_value;

	return 1;	
}


zestcfi_func_ptr_t* zest_cfi_dispatch_c(ElfW(Addr) target)
{
	write_ptr(target);
	write_str(" -- ");

	ElfW(Addr) zestcfi_addr=target;
	int res=dl_iterate_phdr(dl_iterate_phdr_callback,(void*)&zestcfi_addr);

	if(res==0)
	{
		write_str("Could not find zestcfi for target library.  Allowing control transfer.\n");
		return NULL;
	}
	else
	{
		write_str("Found zest-cfi_dispatcher.  Control transfer will be checked.\n");
		zestcfi_func_ptr_t zest_cfi_ptr = (zestcfi_func_ptr_t) zestcfi_addr;
		return zestcfi_addr;
	}
}
