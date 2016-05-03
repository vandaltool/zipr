
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

	LoadEhdr(fin);
	LoadPhdrs(fin);
	CreateNewPhdrs(min_addr);
	CreateNewEhdr();


	WriteElf(fout);

	// cannot do shared objects (yet)
	// to-do:  allow phdrs to go after segments.
	// to-do: check to see if ehdrs/phdrs fit on the first page of the exe for space saving
	if(min_addr<PAGE_SIZE)
		return;



	virtual_offset_t file_header_page_addr=0;
#if 0
	if(page_align(min_addr)+GetFileHeaderSize() + GetSegHeadersSize() < min_addr)
		file_header_page_addr=page_align(min_addr);	
	else 
		file_header_page_addr=page_align(min_addr)-PAGE_SIZE;	
#else
	// always pre-allocate a page.
	file_header_page_addr=page_align(min_addr)-PAGE_SIZE;	
#endif

	

	

		

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
			cout<<"Writing scoop "<<scoop->GetName()<<" to page: "<<hex<<i<<", perms="<<scoop->getRawPerms()<<endl;
			pagemap[i].union_permissions(scoop->getRawPerms());
			for(int j=0;j<PAGE_SIZE;j++)
			{
				// get the data out of the scoop and put it into the page map.
				virtual_offset_t offset=i-start_addr+j;
				if(offset<scoop->GetContents().size())
					pagemap[i].data[j]=scoop->GetContents()[ offset ]; 
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


