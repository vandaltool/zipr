
#include <zipr_all.h>
#include <libIRDB-core.hpp> 
#include <Rewrite_Utility.hpp>
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
    
#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"
#include "targ-config.h"
#include "beaengine/BeaEngine.h"

using namespace libIRDB;
using namespace std;
using namespace zipr;
using namespace ELFIO;


#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif


#if 0
template <class T> static T page_align(T& in)
{

	return in&~(PAGE_SIZE-1);
}
#endif


void ElfWriter::Write(const ELFIO::elfio *elfiop, FileIR_t* firp, const string &out_file, const string &infile)
{

	FILE* fin=fopen(infile.c_str(), "r");
	FILE* fout=fopen(out_file.c_str(), "w");
	assert(fin && fout);

	CreatePagemap(elfiop, firp, out_file);
	CreateSegmap(elfiop, firp, out_file);
	//SortSegmap();
	virtual_offset_t min_addr=DetectMinAddr(elfiop, firp, out_file);
	virtual_offset_t max_addr=DetectMaxAddr(elfiop, firp, out_file);

	LoadEhdr(fin);
	LoadPhdrs(fin);
	CreateNewPhdrs(min_addr,max_addr);


	WriteElf(fout);

}

virtual_offset_t ElfWriter::DetectMinAddr(const ELFIO::elfio *elfiop, FileIR_t* firp, const string &out_file)
{
	virtual_offset_t min_addr=(*(firp->GetDataScoops().begin()))->GetStart()->GetVirtualOffset();
	for(DataScoopSet_t::iterator it=firp->GetDataScoops().begin(); it!=firp->GetDataScoops().end(); ++it)
	{
		DataScoop_t* scoop=*it;

		if(scoop->GetStart()->GetVirtualOffset() < min_addr)
			min_addr=scoop->GetStart()->GetVirtualOffset();

	}
	return min_addr;

}

virtual_offset_t ElfWriter::DetectMaxAddr(const ELFIO::elfio *elfiop, FileIR_t* firp, const string &out_file)
{
	virtual_offset_t max_addr=(*(firp->GetDataScoops().begin()))->GetEnd()->GetVirtualOffset();
	for(DataScoopSet_t::iterator it=firp->GetDataScoops().begin(); it!=firp->GetDataScoops().end(); ++it)
	{
		DataScoop_t* scoop=*it;

		if(scoop->GetEnd()->GetVirtualOffset() > max_addr)
			max_addr=scoop->GetStart()->GetVirtualOffset();

	}
	return max_addr;

}

void ElfWriter::CreatePagemap(const ELFIO::elfio *elfiop, FileIR_t* firp, const string &out_file)
{

	for(DataScoopSet_t::iterator it=firp->GetDataScoops().begin(); it!=firp->GetDataScoops().end(); ++it)
	{
		DataScoop_t* scoop=*it;

		AddressID_t* scoop_addr=scoop->GetStart();
		virtual_offset_t start_addr=scoop_addr->GetVirtualOffset();
		virtual_offset_t end_addr=scoop->GetEnd()->GetVirtualOffset();

		// we'll deal with unpinned scoops later.
		if(scoop_addr->GetVirtualOffset()==0)
		{
			assert(0); // none for now?
			continue;
		}

		for(virtual_offset_t i=page_align(start_addr); i<end_addr; i+=PAGE_SIZE)
		{
			cout<<"Writing scoop "<<scoop->GetName()<<" to page: "<<hex<<i<<", perms="<<scoop->getRawPerms()
			    << " start="<<hex<< scoop->GetStart()->GetVirtualOffset()
			    << " end="<<hex<< scoop->GetEnd()->GetVirtualOffset() <<endl;
			pagemap[i].union_permissions(scoop->getRawPerms());
			for(int j=0;j<PAGE_SIZE;j++)
			{
				// get the data out of the scoop and put it into the page map.
				virtual_offset_t offset=i-start_addr+j;
				if(offset<scoop->GetContents().size())
				{
					pagemap[i].data[j]=scoop->GetContents()[ offset ]; 
					pagemap[i].inuse[j]=true;
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
	// init some segment vars.
	virtual_offset_t segstart=pagemap.begin()->first;
	PageData_t segperms=pagemap.begin()->second;
	virtual_offset_t segend=segstart+PAGE_SIZE;
	virtual_offset_t initend=segstart;
	if(pagemap.begin()->second.is_zero_initialized())
		initend=segstart;
	else
		initend=segend;

	PageMap_t::iterator it=pagemap.begin(); 
	++it;	// handled first one above.

	for( /* init'd above */; it!=pagemap.end(); ++it)
	{
		// grab page address and perms
		virtual_offset_t pagestart=it->first;
		const PageData_t &perms=it->second;


		// if we switch perms, or skip a page 
		if( (perms.m_perms!=segperms.m_perms) || (segend!=pagestart))
		{
			LoadSegment_t *seg=new LoadSegment_t;
			seg->memsz=segend-segstart;
			seg->filesz=initend-segstart;
			seg->start_page=segstart;
			seg->m_perms=segperms.m_perms;
			segvec.push_back(seg);

			cout<<"Found segment "<<hex<<segstart<<"-"<<(segend-1)<<", perms="<<segperms.m_perms<<", memsz="<<seg->memsz<<", filesz="<<seg->filesz<<endl;

			segperms=perms;
			segstart=pagestart;
			segend=segstart+PAGE_SIZE;
			if(perms.is_zero_initialized())
				initend=segstart;
			else
				initend=segend;

		}
		else
		{
			// else, same permission and next page, extend segment. 
			segend=pagestart+PAGE_SIZE;
			if(!perms.is_zero_initialized())
				initend=pagestart+PAGE_SIZE;
		}
		
	}

	// make sure we print the last one
	LoadSegment_t *seg=new LoadSegment_t;
	seg->memsz=segend-segstart;
	seg->filesz=initend-segstart;
	seg->start_page=segstart;
	seg->m_perms=segperms.m_perms;
	segvec.push_back(seg);

	cout<<"Found segment "<<hex<<segstart<<"-"<<(segend-1)<<", perms="<<segperms.m_perms<<", memsz="<<seg->memsz<<", filesz="<<seg->filesz<<endl;
	
}







template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr>
void ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr>::LoadEhdr(FILE* fin) 
{
	fseek(fin,0,SEEK_SET);
	fread(&ehdr,sizeof(ehdr), 1, fin);
};
template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr>
void ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr>::LoadPhdrs(FILE* fin) 
{
	fseek(fin,ehdr.e_phoff,SEEK_SET);
	phdrs.resize(ehdr.e_phnum);
	for(unsigned int i=0;i<phdrs.size();i++)
	{
		fread(&phdrs[i], sizeof(phdrs[i]), 1, fin);
	}
};
template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr>
void ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr>::CreateNewPhdrs(const libIRDB::virtual_offset_t &min_addr, const libIRDB::virtual_offset_t &max_addr) 
{
	
	if(CreateNewPhdrs_GapAllocate(min_addr, max_addr))
		return;
	else if(CreateNewPhdrs_FirstPageAllocate(min_addr, max_addr))
		return;
	else if(CreateNewPhdrs_PreAllocate(min_addr, max_addr))
		return;
	else if(CreateNewPhdrs_PostAllocate(min_addr, max_addr))
		return;
	else
		assert(0);
	
}
template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr>
bool ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr>::CreateNewPhdrs_PostAllocate(const libIRDB::virtual_offset_t &min_addr, const libIRDB::virtual_offset_t &max_addr) 
{
	// post allocation not enabled, yet.
	return false;
}
template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr>
bool ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr>::CreateNewPhdrs_FirstPageAllocate(const libIRDB::virtual_offset_t &min_addr, const libIRDB::virtual_offset_t &max_addr) 
{
	// check to see if there's room on the first page for 
	unsigned int phdr_size=DetermineMaxPhdrSize();
	if(page_align(min_addr)+sizeof(T_Elf_Ehdr)+phdr_size > min_addr)
		return false;
	// this is an uncommon case -- we are typically adding
	// segments and so the segment map won't fit on the first page.
	// if this assertion hits, email hiser@virginia.edu and attack input pgm,
	// then convert this to a return false to avoid assertion until he fixes it;
	assert(0);
}
template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr>
bool ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr>::readonly_space_at(const libIRDB::virtual_offset_t addr, const unsigned int size)
{
	for(unsigned int i=0;i<size;i++)
	{
		libIRDB::virtual_offset_t page=page_align(addr+i);
		libIRDB::virtual_offset_t page_offset=addr+i-page;
	
		// page not allocated yet, go ahead and call this byte free.
		if(pagemap.find(page) == pagemap.end())
			continue;

		if(pagemap.at(page).inuse[page_offset])
			return false;

		// check write perms.  4=r, 2=w, 1=x.
		if((pagemap.at(page).m_perms & 0x2) != 0)
			return false;
	}
	return true;
}
template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr>
int ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr>::locate_segment_index(const libIRDB::virtual_offset_t addr)
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
template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr>
unsigned int ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr>::count_filesz_to_seg(unsigned int seg)
{
	unsigned int filesz=0;
	// segment's are sorted by address.
	for(unsigned int i=0;i<seg;i++)
	{
		filesz+=segvec[i]->filesz;
	}
	return filesz;
}
template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr>
bool ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr>::CreateNewPhdrs_GapAllocate(const libIRDB::virtual_offset_t &min_addr, const libIRDB::virtual_offset_t &max_addr) 
{
	/* for shared objects, we need the PHDR file offset to be equal to the  
	 * memory offset because the kernel passes base_address+Ehdr::ph_off via
	 * the auxv array to ld.so.  Where ld.so then uses that as an address.
	 */


	// first, find the first free space that's big enough.
	unsigned int phdr_size=DetermineMaxPhdrSize();
	libIRDB::virtual_offset_t new_phdr_addr=0;
	for(unsigned int i=min_addr;i<max_addr; i++)
	{
		if(readonly_space_at(i,phdr_size))
		{
			new_phdr_addr=i;
			break;
		}
	
	}

	// find segment
	int new_phdr_segment_index=locate_segment_index(new_phdr_addr);

	// if there's no segment for the start, we'll have to allocate a page anyhow.  just use the _Preallocate routine.
	if(new_phdr_segment_index==-1)
		return false;


	// i can't even convience a multiple-page phdr.
	assert(phdr_size<PAGE_SIZE);

	// verify that the segment can be extended.
	int pages_to_extend=false;	// extend the segment by a page.
	for(unsigned int i=0;i<phdr_size; i++)
	{
		libIRDB::virtual_offset_t this_addr=new_phdr_addr+i;
		libIRDB::virtual_offset_t this_page=page_align(this_addr);
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
		return false;
	}

	// if we get here, we've found a spot for the PHDR.

	// mark the bytes of the pages as readable, and in-use.
	for(unsigned int i=0;i<phdr_size; i++)
	{
		libIRDB::virtual_offset_t this_addr=new_phdr_addr+i;
		libIRDB::virtual_offset_t this_page=page_align(this_addr);
		libIRDB::virtual_offset_t this_offset=this_addr-this_page;
		pagemap[this_page].inuse[this_offset]=true;
		pagemap[this_page].union_permissions(0x4); // add read permission
	}
	segvec[new_phdr_segment_index]->filesz+=(PAGE_SIZE*pages_to_extend);
	segvec[new_phdr_segment_index]->memsz+=(PAGE_SIZE*pages_to_extend);

	unsigned int fileoff=count_filesz_to_seg(new_phdr_segment_index);
	fileoff+=(new_phdr_addr-segvec[new_phdr_segment_index]->start_page);

	return CreateNewPhdrs_internal(min_addr,max_addr,0x0,false,fileoff, new_phdr_addr);

}

template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr>
bool ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr>::CreateNewPhdrs_PreAllocate(const libIRDB::virtual_offset_t &min_addr, const libIRDB::virtual_offset_t &max_addr) 
{
	libIRDB::virtual_offset_t new_phdr_addr=(T_Elf_Addr)page_align(min_addr)-PAGE_SIZE+sizeof(T_Elf_Ehdr);
	return CreateNewPhdrs_internal(min_addr,max_addr,0x1000,true, sizeof(T_Elf_Ehdr), new_phdr_addr);
}
template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr>
bool ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr>::CreateNewPhdrs_internal(
	const libIRDB::virtual_offset_t &min_addr, 
	const libIRDB::virtual_offset_t &max_addr,
	const int &first_seg_file_offset,
	const bool &add_pt_load_for_phdr,
	const size_t phdr_map_offset,
	libIRDB::virtual_offset_t new_phdr_addr
	) 
{
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
		std::cout<<"Assigning load[i].ph_offset="<<std::hex<<fileoff<<std::endl;
		thisphdr.p_vaddr = (T_Elf_Addr)segvec[i]->start_page; 
		thisphdr.p_paddr = (T_Elf_Addr)segvec[i]->start_page; 
		thisphdr.p_filesz = (ELFIO::Elf_Xword)segvec[i]->filesz; 
		thisphdr.p_memsz = (ELFIO::Elf_Xword)segvec[i]->memsz; 
		thisphdr.p_align=  0x1000;

		new_phdrs.push_back(thisphdr);

		// advance file pointer.
		fileoff+=segvec[i]->filesz;
	}


	// go through orig. phdrs any copy any that aren't of type pt_load or pt_hdr
	for(unsigned int i=0;i<phdrs.size();i++)
	{
		// skip any load sections, the irdb tells us what to load.
		if(phdrs[i].p_type == PT_LOAD)
			continue;

		// skip phdr section.
		if(phdrs[i].p_type == PT_PHDR)
			continue;
		
		T_Elf_Phdr newphdr=phdrs[i];

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
		// specify a load section for the new program's header and phdr
		T_Elf_Phdr newheaderphdr;
		memset(&newheaderphdr,0,sizeof(newheaderphdr));
		newheaderphdr.p_type = PT_LOAD;
		newheaderphdr.p_flags =(ELFIO::Elf_Word)4;
		newheaderphdr.p_offset =0;
		newheaderphdr.p_vaddr =(T_Elf_Addr)page_align(min_addr)-PAGE_SIZE;
		newheaderphdr.p_paddr =(T_Elf_Addr)page_align(min_addr)-PAGE_SIZE;
		newheaderphdr.p_filesz =(ELFIO::Elf_Xword)PAGE_SIZE;
		newheaderphdr.p_memsz =(ELFIO::Elf_Xword)PAGE_SIZE;
		newheaderphdr.p_align =0x1000;
		new_phdrs.insert(new_phdrs.begin(),newheaderphdr);
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
	newphdrphdr.p_filesz =(ELFIO::Elf_Xword)(new_phdrs.size()+1)*GetSegHeaderSize();
	newphdrphdr.p_memsz =(ELFIO::Elf_Xword)(new_phdrs.size()+1)*GetSegHeaderSize();
	newphdrphdr.p_align =0x1000;
	new_phdrs.insert(new_phdrs.begin(),newphdrphdr);

	// record the new ehdr.
	new_ehdr=ehdr;
	new_ehdr.e_phoff=phdr_map_offset;
	new_ehdr.e_shoff=0;
	new_ehdr.e_shnum=0;
	new_ehdr.e_phnum=new_phdrs.size();
	// new_ehdr.e_phoff=sizeof(new_ehdr);
	new_ehdr.e_shstrndx=0;
	return true;
}
template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr>
void ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr>::WriteElf(FILE* fout)
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
				if(j+PAGE_SIZE < new_phdrs[i].p_filesz)
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

template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr>
unsigned int ElfWriterImpl<T_Elf_Ehdr,T_Elf_Phdr,T_Elf_Addr>::DetermineMaxPhdrSize()
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
	phdr_count+=segvec.size();
	
	// add 2 more sections that 1) of type PT_PHDR, and 2) a possible PT_LOAD section to load the phdr.
	// worst case is we allocate an entire new page for it.
	phdr_count+=2;		

	return phdr_count*sizeof(T_Elf_Phdr);
}


//  explicit instantation of methods for 32- and 64-bit classes.
template class ElfWriterImpl<ELFIO::Elf64_Ehdr, ELFIO::Elf64_Phdr, ELFIO::Elf64_Addr>;
template class ElfWriterImpl<ELFIO::Elf32_Ehdr, ELFIO::Elf32_Phdr, ELFIO::Elf32_Addr>;

