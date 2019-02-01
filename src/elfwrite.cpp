
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
    
#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"

using namespace IRDB_SDK;
using namespace std;
using namespace zipr;
using namespace ELFIO;


static inline uintptr_t page_round_up(uintptr_t x)
{
        return  ( (((uintptr_t)(x)) + PAGE_SIZE-1)  & (~(PAGE_SIZE-1)) );
}



void ElfWriter::Write(const ELFIO::elfio *elfiop, FileIR_t* firp, const string &out_file, const string &infile)
{

	FILE* fin=fopen(infile.c_str(), "r");
	FILE* fout=fopen(out_file.c_str(), "w");
	assert(fin && fout);

	CreatePagemap(elfiop, firp, out_file);
	CreateSegmap(elfiop, firp, out_file);
	//SortSegmap();
	VirtualOffset_t min_addr=DetectMinAddr(elfiop, firp, out_file);
	VirtualOffset_t max_addr=DetectMaxAddr(elfiop, firp, out_file);

	LoadEhdr(fin);
	LoadPhdrs(fin);
	CreateNewPhdrs(min_addr,max_addr);


	WriteElf(fout);

	if( m_write_sections ) 
		AddSections(fout);

}

VirtualOffset_t ElfWriter::DetectMinAddr(const ELFIO::elfio *elfiop, FileIR_t* firp, const string &out_file)
{
	VirtualOffset_t min_addr=(*(firp->getDataScoops().begin()))->getStart()->getVirtualOffset();
	for(DataScoopSet_t::iterator it=firp->getDataScoops().begin(); it!=firp->getDataScoops().end(); ++it)
	{
		DataScoop_t* scoop=*it;

		if(scoop->getStart()->getVirtualOffset() < min_addr)
			min_addr=scoop->getStart()->getVirtualOffset();

	}
	return min_addr;

}

VirtualOffset_t ElfWriter::DetectMaxAddr(const ELFIO::elfio *elfiop, FileIR_t* firp, const string &out_file)
{
	VirtualOffset_t max_addr=(*(firp->getDataScoops().begin()))->getEnd()->getVirtualOffset();
	for(DataScoopSet_t::iterator it=firp->getDataScoops().begin(); it!=firp->getDataScoops().end(); ++it)
	{
		DataScoop_t* scoop=*it;

		if(scoop->getEnd()->getVirtualOffset() > max_addr)
			max_addr=scoop->getEnd()->getVirtualOffset();

	}
	return max_addr;

}

void ElfWriter::CreatePagemap(const ELFIO::elfio *elfiop, FileIR_t* firp, const string &out_file)
{

	for(DataScoopSet_t::iterator it=firp->getDataScoops().begin(); it!=firp->getDataScoops().end(); ++it)
	{
		DataScoop_t* scoop=*it;

		AddressID_t* scoop_addr=scoop->getStart();
		VirtualOffset_t start_addr=scoop_addr->getVirtualOffset();
		VirtualOffset_t end_addr=scoop->getEnd()->getVirtualOffset();

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

void ElfWriter::SortSegmap()
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

void ElfWriter::CreateSegmap(const ELFIO::elfio *elfiop, FileIR_t* firp, const string &out_file)
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
/*
			LoadSegment_t *seg=new LoadSegment_t;
			seg->memsz=segend-segstart;
			seg->filesz=initend-segstart;
			seg->start_page=segstart;
			seg->m_perms=segperms.m_perms;
*/
			const auto seg=new LoadSegment_t(initend-segstart, segend-segstart, 0, segstart,segperms.m_perms);
			segvec.push_back(seg);

			cout<<"Found segment "<<hex<<segstart<<"-"<<(segend-1)<<", perms="<<segperms.m_perms<<", memsz="<<seg->memsz<<", filesz="<<seg->filesz<<endl;

			segperms=perms;
			segstart=pagestart;
			segend=segstart+PAGE_SIZE;

			update_initend(perms);
/*
			if( should_bss_optimize(perms) ) // perms.is_zero_initialized() && m_bss_opts)
				initend=segstart;
			else
				initend=segend;
*/

		}
		else
		{
			// else, same permission and next page, extend segment. 
			segend=pagestart+PAGE_SIZE;
			if(! should_bss_optimize(perms) ) // !perms.is_zero_initialized() || ! m_bss_opts)
				initend=pagestart+PAGE_SIZE;
		}
		
	}

	// make sure we print the last one
/*
	LoadSegment_t *seg=new LoadSegment_t;
	seg->memsz=segend-segstart;
	seg->filesz=initend-segstart;
	seg->start_page=segstart;
	seg->m_perms=segperms.m_perms;
*/
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

#ifdef CGC
// 
// Type           Offset   VirtAddr   PhysAddr   FileSiz MemSiz  Flg Align
// LOAD           0x000f20 0x00000000 0x00000000 0x00000 0x00000       0x1000 (edited)
	// create 0-size headre
	std::cout<<"New phdrs at: "<<std::hex<<new_phdr_addr<<std::endl;
	T_Elf_Phdr aqphdr;
	memset(&aqphdr,0,sizeof(aqphdr));
	aqphdr.p_type = PT_LOAD;
	aqphdr.p_offset =phdr_map_offset; 
	aqphdr.p_align =0x1000;
	new_phdrs.insert(new_phdrs.begin(),aqphdr);
#endif

	// record the new ehdr.
	new_ehdr=ehdr;
	new_ehdr.e_phoff=phdr_map_offset;
	new_ehdr.e_shoff=0;
	new_ehdr.e_shnum=0;
	new_ehdr.e_phnum=new_phdrs.size();
	// new_ehdr.e_phoff=sizeof(new_ehdr);
	new_ehdr.e_shstrndx=0;

	update_phdr_for_scoop_sections(m_firp);


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
					// don't write 0's at end of last page 
					int k=0;
					for (k=PAGE_SIZE-1;k>=0;k--)
					{
						if(page.data[k]!=0)
							break;
					}
					std::cout<<"phdr["<<std::dec<<loadcnt<<"] optimizing last page write to size k="<<std::hex<<k<<std::endl;
					fwrite(page.data.data(), k+1, 1, fout);
				}
		
			}
		}
	}

	// write the header.
	fseek(fout,0,SEEK_SET);
	fwrite(&new_ehdr, sizeof(new_ehdr), 1, fout);
	// write the phdrs, which may be part of a segment written above.
	std::cout<<"Writing segment headers at "<<std::hex<<new_ehdr.e_phoff
		<<", size="<<new_phdrs.size()*sizeof(new_phdrs[0])<<std::endl;
	fseek(fout,new_ehdr.e_phoff,SEEK_SET);
	fwrite(new_phdrs.data(), sizeof(new_phdrs[0]), new_phdrs.size(), fout);
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
	for_each(m_firp->getDataScoops().begin(), m_firp->getDataScoops().end(), [&](DataScoop_t* scoop)
	{
		strtab.AddString(scoop->getName());
	});


	string zipr_symtab=".scoop_symtab";
	strtab.AddString(zipr_symtab);
	string null_symtab="nullptr";
	strtab.AddString(null_symtab);


	// locate a file offset for each scoop by examining the output phdrs.
	for_each(m_firp->getDataScoops().begin(), m_firp->getDataScoops().end(), [&](DataScoop_t* scoop)
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
	});

	T_Elf_Shdr null_shdr;
	memset(&null_shdr,0,sizeof(null_shdr));
	null_shdr.sh_type=SHT_nullptr;
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
	for_each(m_firp->getDataScoops().begin(), m_firp->getDataScoops().end(), [&](DataScoop_t* scoop)
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
	});
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
	fwrite(shdrs.data(), sizeof(T_Elf_Shdr), shdrs.size(), fout);

	new_ehdr.e_shentsize=sizeof(T_Elf_Shdr);
	new_ehdr.e_shnum=shdrs.size();
	new_ehdr.e_shstrndx=shdrs.size()-1;	// symtab was added last.
	new_ehdr.e_shoff=shdr_file_pos;

	// rewrite the file header so that sections are listed.
	fseek(fout,0,SEEK_SET);
	fwrite(&new_ehdr, sizeof(new_ehdr),1,fout);
}

//  explicit instantation of methods for 32- and 64-bit classes.
template class ElfWriterImpl<ELFIO::Elf64_Ehdr, ELFIO::Elf64_Phdr, ELFIO::Elf64_Addr, ELFIO::Elf64_Shdr, ELFIO::Elf64_Sym, ELFIO::Elf64_Rel, ELFIO::Elf64_Rela, ELFIO::Elf64_Dyn>;
template class ElfWriterImpl<ELFIO::Elf32_Ehdr, ELFIO::Elf32_Phdr, ELFIO::Elf32_Addr, ELFIO::Elf32_Shdr, ELFIO::Elf32_Sym, ELFIO::Elf32_Rel, ELFIO::Elf32_Rela, ELFIO::Elf32_Dyn>;

