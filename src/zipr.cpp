/***************************************************************************
 * Copyright (c)  2014  Zephyr Software LLC. All rights reserved.
 *
 * This software is furnished under a license and/or other restrictive
 * terms and may be used and copied only in accordance with such terms
 * and the inclusion of the above copyright notice. This software or
 * any other copies thereof may not be provided or otherwise made
 * available to any other person without the express written consent
 * of an authorized representative of Zephyr Software LCC. Title to,
 * ownership of, and all rights in the software is retained by
 * Zephyr Software LCC.
 *
 * Zephyr Software LLC. Proprietary Information
 *
 * Unless otherwise specified, the information contained in this
 * directory, following this legend, and/or referenced herein is
 * Zephyr Software LLC. (Zephyr) Proprietary Information.
 *
 * CONTACT
 *
 * For technical assistance, contact Zephyr Software LCC. at:
 *
 *
 * Zephyr Software, LLC
 * 2040 Tremont Rd
 * Charlottesville, VA 22911
 * 
 * E-mail: jwd@zephyr-software.com
 **************************************************************************/

#include <zipr_all.h>
#include <libIRDB-core.hpp>
#include <iostream>
#include <stdlib.h>
#include <string.h>
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


int find_magic_segment_index(ELFIO::elfio *elfiop);


static std::ifstream::pos_type filesize(const char* filename)
{
    	std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);

	if(!in.is_open())
	{
		cerr<<"Cannot open file: "<<filename<<endl;
		throw string("Cannot open file\n");
	}
   	return in.tellg();
}


void Zipr_t::CreateBinaryFile(const std::string &name)
{
	m_stats = new Stats_t();

/* have to figure this out.  we'll want to really strip the binary
 * but also remember the strings. 
 */
#ifdef CGC
	string cmd=string("cp ")+name+" "+name+".stripped ; ~/Downloads/ELFkickers-3.0a/sstrip/sstrip "+name+".stripped";
	printf("Attempting: %s\n", cmd.c_str());
	if(-1 == system(cmd.c_str()))
	{
		perror(__FUNCTION__);
	}
#endif


	// create ranges, including extra range that's def. big enough.
	FindFreeRanges(name);

	// add pinned instructions
	AddPinnedInstructions();

	// reserve space for pins
	ReservePinnedInstructions();

	// Emit instruction immediately?

	if (m_opts.IsEnabledOptimization(Optimizations_t::OptimizationFallthroughPinned))
	{
		OptimizePinnedFallthroughs();
	}

	PreReserve2ByteJumpTargets();

	// expand 2-byte pins into 4-byte pins
	ExpandPinnedInstructions();

	while (!two_byte_pins.empty()) 
	{
		/*
		 * Put down the five byte targets
		 * for two byte jumps, if any exist.
		 */
		printf("Going to Fix2BytePinnedInstructions.\n");
		Fix2BytePinnedInstructions();

		/*
		 * If there are still two byte pins, 
		 * try the dance again.
		 */
		if (!two_byte_pins.empty())
		{
			printf("Going to Re PreReserve2ByteJumpTargets.\n");
			PreReserve2ByteJumpTargets();
		}
	}


	// Convert all 5-byte pins into full fragments
	OptimizePinnedInstructions();

	// now that pinning is done, start emitting unpinnned instructions, and patching where needed.
	PlopTheUnpinnedInstructions();

	// now that all instructions are put down, we can figure out where the callbacks for this file wil go.
	// go ahead and update any callback sites with the new locations 
	UpdateCallbacks();

	m_stats->total_free_ranges = memory_space.GetRangeCount();

	// write binary file to disk 
	OutputBinaryFile(name);

	// print relevant information
	PrintStats();
}

static bool in_same_segment(ELFIO::section* sec1, ELFIO::section* sec2, ELFIO::elfio* elfiop)
{
	ELFIO::Elf_Half n = elfiop->segments.size();
	for ( ELFIO::Elf_Half i = 0; i < n; ++i ) 
	{
		uintptr_t segstart=elfiop->segments[i]->get_virtual_address();
		uintptr_t segsize=elfiop->segments[i]->get_file_size();

		/* sec1 in segment i? */
		if(segstart <= sec1->get_address() && sec1->get_address() < (segstart+segsize))
		{
			/* return true if sec2 also in segment */
			/* else return false */
			return (segstart <= sec2->get_address() && sec2->get_address() < (segstart+segsize));
		}
	}

	return false;
}


//
// check if there's padding we can use between this section and the next section.
//
RangeAddress_t Zipr_t::extend_section(ELFIO::section *sec, ELFIO::section *next_sec)
{
	RangeAddress_t start=sec->get_address();
	RangeAddress_t end=sec->get_size()+start;
	if( (next_sec->get_flags() & SHF_ALLOC) != 0 && in_same_segment(sec,next_sec,elfiop))
	{
		end=next_sec->get_address()-1;
		cout<<"Extending range to "<<std::hex<<end<<endl;
	}
	return end;
}

void Zipr_t::FindFreeRanges(const std::string &name)
{
	/* use ELFIO to load the sections */
	elfiop=new ELFIO::elfio;

	assert(elfiop);
	elfiop->load(name);
//	ELFIO::dump::header(cout,*elfiop);
	ELFIO::dump::section_headers(cout,*elfiop);

	RangeAddress_t last_end=0;
	RangeAddress_t max_addr=0;

	std::map<RangeAddress_t, int> ordered_sections;

	/*
	 * Make an ordered list of the sections
	 * by their starting address.
	 */
	ELFIO::Elf_Half n = elfiop->sections.size();
	for ( ELFIO::Elf_Half i = 0; i < n; ++i ) 
	{ 
		section* sec = elfiop->sections[i];
		assert(sec);
		ordered_sections.insert(std::pair<RangeAddress_t,int>(sec->get_address(), i));
	}

	std::map<RangeAddress_t, int>::iterator it = ordered_sections.begin();
	for (;it!=ordered_sections.end();) 
	{ 
		section* sec = elfiop->sections[it->second];
		assert(sec);

		RangeAddress_t start=sec->get_address();
		RangeAddress_t end=sec->get_size()+start;

		if(m_opts.GetVerbose())
			printf("Section %s:\n", sec->get_name().c_str());

#if 1

		std::map<RangeAddress_t, int>::iterator next_sec_it = it;
		++next_sec_it;
		if(next_sec_it!=ordered_sections.end())
		{
			section* next_sec = elfiop->sections[next_sec_it->second];
			end=extend_section(sec,next_sec);
		}
#endif




		++it;
		if (false)
		//if ((++it) != ordered_sections.end())
		{
			/*
			 * TODO: This works. However, the updated
			 * section size is not properly handled
			 * in OutputBinaryFile. So, it is disabled
			 * until that is handled.
			 */
			section *next_section = elfiop->sections[it->second];

			printf("Using %s as the next section (%p).\n", 
				next_section->get_name().c_str(), 
				(void*)next_section->get_address());
			printf("Modifying the section end. Was %p.", (void*)end);

			end = next_section->get_address() - 1;
			sec->set_size(end - start);
			
			printf(". Is %p.\n", (void*)end);

		}

		if(m_opts.GetVerbose())
			printf("max_addr is %p, end is %p\n", (void*)max_addr, (void*)end);
		if(start && end>max_addr)
		{
			if(m_opts.GetVerbose())
				printf("new max_addr is %p\n", (void*)max_addr);
			max_addr=end;
		}

		if( (sec->get_flags() & SHF_ALLOC) ==0 )
			continue;


		if((sec->get_flags() & SHF_EXECINSTR))
		{
			assert(start>last_end);
			last_end=end;
			if(m_opts.GetVerbose())
				printf("Adding free range 0x%p to 0x%p\n", (void*)start,(void*)end);
			memory_space.AddFreeRange(Range_t(start,end));
		}
	}

#define PAGE_SIZE 4096
#define PAGE_ROUND_UP(x) ( (((uintptr_t)(x)) + PAGE_SIZE-1)  & (~(PAGE_SIZE-1)) )

	// now that we've looked at the sections, add a (mysterious) extra section in case we need to overflow 
	// the sections existing in the ELF.
// skip round up?  not needed if callbacks are PIC/PIE.
#ifndef CGC
	RangeAddress_t new_free_page=PAGE_ROUND_UP(max_addr);
#else
	int i=find_magic_segment_index(elfiop);
	RangeAddress_t bytes_remaining_in_file=(RangeAddress_t)filesize((name+".stripped").c_str())-(RangeAddress_t)elfiop->segments[i]->get_file_offset();
	RangeAddress_t bytes_in_seg=elfiop->segments[i]->get_memory_size();
	RangeAddress_t new_free_page=-1;

	if(bytes_remaining_in_file>=bytes_in_seg)
	{
		cout<<"Note: Found that segment can be extended for free because bss_needed==0"<<endl;
		bss_needed=0;
		new_free_page=elfiop->segments[i]->get_virtual_address()+bytes_remaining_in_file;

		// the end of the file has to be loadable into the bss segment.
		assert(bytes_remaining_in_file==elfiop->segments[i]->get_file_size());
	}
	else
	{
/* experimentally, we add 2 pages using the "add_strata_segment" method. */
/* the "bss_needed" method works pretty well unless there is significantly more than 8k of bss space */
#define BSS_NEEDED_THRESHOLD 8096
		bss_needed=bytes_in_seg-bytes_remaining_in_file;

		if(bss_needed < BSS_NEEDED_THRESHOLD)
		{
			new_free_page=elfiop->segments[i]->get_virtual_address()+bytes_in_seg;
			cout<<"Note: Found that segment can be extended with penalty bss_needed=="<<std::dec<<bss_needed<<endl;
		}
		else
		{
			new_free_page=PAGE_ROUND_UP(max_addr);
			use_stratafier_mode=true;
		}
	}
#endif

	memory_space.AddFreeRange(Range_t(new_free_page,(RangeAddress_t)-1));
	if(m_opts.GetVerbose())
		printf("Adding (mysterious) free range 0x%p to EOF\n", (void*)new_free_page);
	start_of_new_space=new_free_page;
}

void Zipr_t::AddPinnedInstructions()
{

        for(   
                set<Instruction_t*>::const_iterator it=m_firp->GetInstructions().begin();
                it!=m_firp->GetInstructions().end();
                ++it
           )
        {
		Instruction_t* insn=*it;
		assert(insn);

		if(!insn->GetIndirectBranchTargetAddress())
			continue;

		unresolved_pinned_addrs.insert(UnresolvedPinned_t(insn));
	}

}

Instruction_t *Zipr_t::FindPinnedInsnAtAddr(RangeAddress_t addr)
{
	for(
		set<Instruction_t*>::const_iterator it=m_firp->GetInstructions().begin();
		it!=m_firp->GetInstructions().end();
		++it
	)
	{
		RangeAddress_t ibta_addr;
		Instruction_t* insn=*it;
		assert(insn);

		if(!insn->GetIndirectBranchTargetAddress()) {
			continue;
		}
		ibta_addr=(unsigned)insn->
			GetIndirectBranchTargetAddress()->
			GetVirtualOffset();

		if (addr == ibta_addr)
			return insn;
	}
	return NULL;
}

bool Zipr_t::ShouldPinImmediately(Instruction_t *upinsn)
{
	DISASM d;
	upinsn->Disassemble(d);
	Instruction_t *pin_at_next_byte = NULL;
	AddressID_t *upinsn_ibta = NULL, *ft_ibta = NULL;

	if(d.Instruction.BranchType==RetType)
		return true;

	upinsn_ibta=upinsn->GetIndirectBranchTargetAddress();
	assert(upinsn_ibta!=NULL);

	if (upinsn->GetFallthrough() != NULL)
		ft_ibta=upinsn->GetFallthrough()->GetIndirectBranchTargetAddress();

	/* careful with 1 byte instructions that have a pinned fallthrough */ 
	if(upinsn->GetDataBits().length()==1)
	{
		if(upinsn->GetFallthrough()==NULL)
			return true;
		ft_ibta=upinsn->GetFallthrough()->GetIndirectBranchTargetAddress();
		if(ft_ibta && (upinsn_ibta->GetVirtualOffset()+1) == ft_ibta->GetVirtualOffset())
			return true;
	}

	/*
	 * lock cmpxchange op1 op2 [pinned at x]
	 * x    x+1        x+2 x+3
	 * 
	 * pin at x and pin at x+1
	 *
	 * x should become nop, put down immediately
	 * x+1 should become the entire lock command.
	 */
	if ((pin_at_next_byte = 
		FindPinnedInsnAtAddr(upinsn_ibta->GetVirtualOffset() + 1)) != NULL)
	{
		if(m_opts.GetVerbose())
			printf("Using pin_at_next_byte special case.\n");
		/*
		 * Because upinsn is longer than 
		 * 1 byte, we must be somehow
		 * pinned into ourselves. Fix!
		 */

		/*
		 * Make pin_at_next_byte look like upinsn.
		 */
		pin_at_next_byte->SetDataBits(upinsn->GetDataBits());
		pin_at_next_byte->SetComment(upinsn->GetComment());
		pin_at_next_byte->SetCallback(upinsn->GetCallback());
		pin_at_next_byte->SetFallthrough(upinsn->GetFallthrough());
		pin_at_next_byte->SetTarget(upinsn->GetTarget());
		/*
		 * Convert upins to nop.
		 */
		string dataBits = upinsn->GetDataBits();
		dataBits.resize(1);
		dataBits[0] = 0x90;
		upinsn->SetDataBits(dataBits);

		return true;
	}
	return false;
}

void Zipr_t::OptimizePinnedFallthroughs()
{
	for(set<UnresolvedPinned_t>::const_iterator it=two_byte_pins.begin();
		it!=two_byte_pins.end();
		)
	{
		UnresolvedPinned_t up=*it;
		Instruction_t *up_insn = up.GetInstruction();
		AddressID_t *up_ibta = NULL, *ft_ibta = NULL;

		up_ibta=up_insn->GetIndirectBranchTargetAddress();
		assert(up_ibta!=NULL);

		if (up_insn->GetFallthrough() != NULL)
			ft_ibta=up_insn->GetFallthrough()->GetIndirectBranchTargetAddress();

		/*
		 * has a fallthrough and
		 * that fallthrough is next and
		 * that fallthrough is pinned.
		 *
		 * Do NOT use this optimization
		 * in the case that the instruction 
		 * has a target or a callback!
		 */
		if (ft_ibta != NULL &&
		    (up_ibta->GetVirtualOffset() + up_insn->GetDataBits().length()) == 
				 ft_ibta->GetVirtualOffset() &&
			  (up_insn->GetCallback()=="") &&
			  (!up_insn->GetTarget()))
		{
			if (m_opts.GetVerbose())
				printf("Emitting pinned instruction (0x%p) "
				       "with pinned fallthrough next.\n",
				       (void*)up_ibta->GetVirtualOffset());
			m_stats->Hit(Optimizations_t::OptimizationFallthroughPinned);

			/*
			 * Unreserve any reserved space for this instructions.
			 */
			for (int j = up.GetRange().GetStart(); j<up.GetRange().GetEnd(); j++)
			{
				memory_space.MergeFreeRange(j);
			}
			up.SetRange(Range_t(0,0));
			PlopInstruction(up_insn,up_ibta->GetVirtualOffset());
			two_byte_pins.erase(it++);
		}
		else
		{
			m_stats->Missed(Optimizations_t::OptimizationFallthroughPinned);
			it++;
		}
	}
}

void Zipr_t::PreReserve2ByteJumpTargets()
{
	for(set<UnresolvedPinned_t>::const_iterator it=two_byte_pins.begin();
		it!=two_byte_pins.end();
		)
	{
		UnresolvedPinned_t up=*it;
		bool found_close_target = false;
		Instruction_t* upinsn=up.GetInstruction();

		RangeAddress_t addr;
		
		if (up.HasUpdatedAddress())
		{
			addr = up.GetUpdatedAddress();
		}
		else
		{
			addr=upinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset();
		}

		/*
		 * Check for near branch instructions
		 * by starting far away!
		 * Note: two byte jump range is 127 bytes, 
		 * but that's from the pc after it's been 
		 * inc, etc. complicated goo. 120 is a 
		 * safe estimate of range.
		 */
		for(int size=5;size>0;size-=3) 
		{
			if(m_opts.GetVerbose())
				printf("Looking for %d-byte jump targets to pre-reserve.\n", size);
			for(int i=120;i>=-120;i--)
			{
				if(memory_space.AreBytesFree(addr+i,size))
				{
					if(m_opts.GetVerbose())
						printf("Found location for 2-byte->%d-byte conversion "
						"(%p-%p)->(%p-%p) (orig: %p)\n", 
						size,
						(void*)addr,
						(void*)(addr+1),
						(void*)(addr+i),
						(void*)(addr+i+size),
						(void*)upinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset());

					up.SetRange(Range_t(addr+i, addr+i+size));
					for (int j = up.GetRange().GetStart(); j<up.GetRange().GetEnd(); j++)
					{
						memory_space.SplitFreeRange(j);
					}
					found_close_target = true;
					break;
				}
			}
			if (found_close_target)
				break;
		}

		if (!found_close_target)
		{
			printf("FATAL: No location for near jump reserved.\n");
			assert(false);
			++it;
		}
		else
		{
			UnresolvedPinned_t new_up = UnresolvedPinned_t(up.GetInstruction(), up.GetRange());
			if (up.HasUpdatedAddress())
			{
				new_up.SetUpdatedAddress(up.GetUpdatedAddress());
			}
			two_byte_pins.erase(it++);
			two_byte_pins.insert(new_up);
				
		}
	}
}

void Zipr_t::ReservePinnedInstructions()
{
	set<UnresolvedPinned_t> reserved_pins;


	/* first, for each pinned instruction, try to put down a jump for the pinned instruction
 	 */
        for(   
		set<UnresolvedPinned_t>::const_iterator it=unresolved_pinned_addrs.begin();
                it!=unresolved_pinned_addrs.end();
                ++it
           )
	{
		UnresolvedPinned_t up=*it;

		Instruction_t* upinsn=up.GetInstruction();
		RangeAddress_t addr=(unsigned)upinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset();

		if(upinsn->GetIndirectBranchTargetAddress()->GetFileID()==BaseObj_t::NOT_IN_DATABASE)
			continue;

		/* sometimes, we need can't just put down a 2-byte jump into the old slot
	   	 * we may need to do alter our technique if there are two consecutive pinned addresses (e.g. 800 and 801).
		 * That case is tricky, as we can't put even a 2-byte jump instruction down. 
		 * so, we attempt to pin any 1-byte instructions with no fallthrough (returns are most common) immediately.
		 * we also attempt to pin any 1-byte insn that falls through to the next pinned address (nops are common).
		 */
		if(ShouldPinImmediately(upinsn))
		{
			if(m_opts.GetVerbose())
				printf("Final pinning %p-%p.  fid=%d\n", (void*)addr, (void*)(addr+upinsn->GetDataBits().size()-1),
				upinsn->GetAddress()->GetFileID());
			for(int i=0;i<upinsn->GetDataBits().size();i++)
			{
				byte_map[addr+i]=upinsn->GetDataBits()[i];
				memory_space.SplitFreeRange(addr+i);
				m_stats->total_other_space++;
			}
			continue;
		}

		char bytes[]={(char)0xeb,(char)0}; // jmp rel8

		if(m_opts.GetVerbose())
		{
			printf("Two-byte Pinning %p-%p.  fid=%d\n", 
				(void*)addr, 
				(void*)(addr+sizeof(bytes)-1),
				upinsn->GetAddress()->GetFileID());
			printf("%s\n", upinsn->GetComment().c_str());
		}

		two_byte_pins.insert(up);
		for(int i=0;i<sizeof(bytes);i++)
		{
			assert(byte_map.find(addr+i) == byte_map.end() );
			byte_map[addr+i]=bytes[i];
			memory_space.SplitFreeRange(addr+i);
		}
	}
}

void Zipr_t::ExpandPinnedInstructions()
{
	/* now, all insns have 2-byte pins.  See which ones we can make 5-byte pins */
	
	for(
		set<UnresolvedPinned_t>::iterator it=two_byte_pins.begin();
		it!=two_byte_pins.end();
		)
	{
		UnresolvedPinned_t up=*it;
		Instruction_t* upinsn=up.GetInstruction();
		RangeAddress_t addr=upinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset();

		char bytes[]={(char)0xe9,(char)0,(char)0,(char)0,(char)0}; // jmp rel8
		bool can_update=memory_space.AreBytesFree(addr+2,sizeof(bytes)-2);
		if(can_update)
		{
			if(m_opts.GetVerbose())
				printf("Found %p can be updated to 5-byte jmp\n", (void*)addr);
			PlopJump(addr);

			/*
			 * Unreserve those bytes that we reserved before!
			 */
			for (int j = up.GetRange().GetStart(); j<up.GetRange().GetEnd(); j++)
			{
				memory_space.MergeFreeRange(j);
			}
			up.SetRange(Range_t(0,0));

			five_byte_pins[up]=addr;
			two_byte_pins.erase(it++);
			m_stats->total_5byte_pins++;
			m_stats->total_trampolines++;
		}
		else
		{
			++it;
			if(m_opts.GetVerbose())
				printf("Found %p can NOT be updated to 5-byte jmp\n", (void*)addr);
			m_stats->total_2byte_pins++;
			m_stats->total_trampolines++;
			m_stats->total_tramp_space+=2;
		}
	}

	printf("Totals:  2-byters=%d, 5-byters=%d\n", (int)two_byte_pins.size(), (int)five_byte_pins.size());
}


void Zipr_t::Fix2BytePinnedInstructions()
{
	for(
		set<UnresolvedPinned_t>::const_iterator it=two_byte_pins.begin();
		it!=two_byte_pins.end();
		)
	{
		UnresolvedPinned_t up=*it;
		Instruction_t* upinsn=up.GetInstruction();
		RangeAddress_t addr;
		
		if (up.HasUpdatedAddress())
		{
			addr = up.GetUpdatedAddress();
		}
		else
		{
			addr=upinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset();
		}

		if (up.HasRange())
		{
			/*
			 * Always clear out the previously reserved space.
			 */
			for (int j = up.GetRange().GetStart(); j<up.GetRange().GetEnd(); j++)
			{
				memory_space.MergeFreeRange(j);
			}

			if (up.GetRange().Is5ByteRange()) 
			{
				if(m_opts.GetVerbose())
					printf("Using previously reserved spot of 2-byte->5-byte conversion "
					"(%p-%p)->(%p-%p) (orig: %p)\n", 
					(void*)addr,
					(void*)(addr+1),
					(void*)(up.GetRange().GetStart()),
					(void*)(up.GetRange().GetEnd()),
					(void*)upinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset());

				five_byte_pins[up] = up.GetRange().GetStart();
				PlopJump(up.GetRange().GetStart());
				PatchJump(addr, up.GetRange().GetStart());

				two_byte_pins.erase(it++);
			}
			else if (up.HasRange() && up.GetRange().Is2ByteRange()) 
			{
				/*
				 * Add jump to the reserved space.
				 * Make an updated up that has a new
				 * "addr" so that addr is handled 
				 * correctly the next time through.
				 *
				 * Ie tell two_byte_pins list that
				 * the instruction is now at the jump
				 * target location.
				 */
				UnresolvedPinned_t new_up = 
					UnresolvedPinned_t(up.GetInstruction());
				new_up.SetUpdatedAddress(up.GetRange().GetStart());

				char bytes[]={(char)0xeb,(char)0}; // jmp rel8
				for(int i=0;i<sizeof(bytes);i++)
				{
					assert(byte_map.find(up.GetRange().GetStart()+i) == byte_map.end() );
					byte_map[up.GetRange().GetStart()+i]=bytes[i];
					memory_space.SplitFreeRange(up.GetRange().GetStart()+i);
					assert(!memory_space.IsByteFree(up.GetRange().GetStart()+i));
				}

				if(m_opts.GetVerbose())
					printf("Patching 2 byte to 2 byte: %p to %p (orig: %p)\n", 
					(void*)addr,
					(void*)up.GetRange().GetStart(),
					(void*)upinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset());

				PatchJump(addr, up.GetRange().GetStart());
				two_byte_pins.erase(it++);
				two_byte_pins.insert(new_up);
			}
		}
		else
		{
			printf("FATAL: Two byte pin without reserved range: %p\n", (void*)addr);
			assert(false);
			it++;
		}
	}
}

void Zipr_t::OptimizePinnedInstructions()
{

	// should only be 5-byte pins by now.

	assert(two_byte_pins.size()==0);


	for(
		std::map<UnresolvedPinned_t,RangeAddress_t>::iterator it=five_byte_pins.begin();
			it!=five_byte_pins.end();
	   )
	{
		RangeAddress_t addr=(*it).second;
		UnresolvedPinned_t up=(*it).first;

		// ideally, we'll try to fill out the pinned 5-byte jump instructions with actual instructions
		// from the program.  That's an optimization.  At the moment, let's just create a patch for each one.

		UnresolvedUnpinned_t uu(up.GetInstruction());
		Patch_t	thepatch(addr,UncondJump_rel32);

		patch_list.insert(pair<UnresolvedUnpinned_t,Patch_t>(uu,thepatch));
		PlopJump(addr);

		DISASM d;
		uu.GetInstruction()->Disassemble(d);

		bool can_optimize=false; // fixme
		if(can_optimize)
		{
			//fixme
		}
		else
		{
			if(m_opts.GetVerbose())
				printf("Converting 5-byte pinned jump at %p-%p to patch to %d:%s\n", 
				(void*)addr,(void*)(addr+4), uu.GetInstruction()->GetBaseID(), d.CompleteInstr);
			m_stats->total_tramp_space+=5;
		}

		// remove and move to next pin
		five_byte_pins.erase(it++);
	}
		
}


void Zipr_t::PlopBytes(RangeAddress_t addr, const char the_byte[], int num)
{
	for(int i=0;i<num;i++)
	{
		PlopByte(addr+i,the_byte[i]);
	}
}

void Zipr_t::PlopByte(RangeAddress_t addr, char the_byte)
{
	if(byte_map.find(addr) == byte_map.end() )
		memory_space.SplitFreeRange(addr);
	byte_map[addr]=the_byte;
}

void Zipr_t::PlopJump(RangeAddress_t addr)
{
	char bytes[]={(char)0xe9,(char)0,(char)0,(char)0,(char)0}; // jmp rel8
	for(int i=0;i<sizeof(bytes);i++)        // don't check bytes 0-1, because they've got the short byte jmp
	{
		PlopByte(addr+i,bytes[i]);
	}
}


void Zipr_t::CallToNop(RangeAddress_t at_addr)
{
	char bytes[]={(char)0x90,(char)0x90,(char)0x90,(char)0x90,(char)0x90}; // nop;nop;nop;nop;nop
	PlopBytes(at_addr,bytes,sizeof(bytes));
}

void Zipr_t::PatchCall(RangeAddress_t at_addr, RangeAddress_t to_addr)
{
	uintptr_t off=to_addr-at_addr-5;

	assert(!memory_space.IsByteFree(at_addr));
	
	switch(byte_map[at_addr])
	{
		case (char)0xe8:	/* 5byte call */
		{
			assert(off==(uintptr_t)off);
			assert(!memory_space.AreBytesFree(at_addr+1,4));

			byte_map[at_addr+1]=(char)(off>> 0)&0xff;
			byte_map[at_addr+2]=(char)(off>> 8)&0xff;
			byte_map[at_addr+3]=(char)(off>>16)&0xff;
			byte_map[at_addr+4]=(char)(off>>24)&0xff;
			break;
		}
		default:
			assert(0);

	}
}

void Zipr_t::PatchJump(RangeAddress_t at_addr, RangeAddress_t to_addr)
{
	uintptr_t off=to_addr-at_addr-2;

	assert(!memory_space.IsByteFree(at_addr));
	
	switch(byte_map[at_addr])
	{
		case (char)0xe9:	/* 5byte jump */
		{
			assert(0);
		}
		case (char)0xeb:	/* 2byte jump */
		{
			assert(off==(uintptr_t)(char)off);

			assert(!memory_space.IsByteFree(at_addr+1));
			byte_map[at_addr+1]=(char)off;
		}
	}
}


#define CALLBACK_TRAMPOLINE_SIZE 9
static int DetermineWorstCaseInsnSize(Instruction_t* insn)
{

	int required_size=0;

	switch(insn->GetDataBits()[0])
	{
		case (char)0x71:
		case (char)0x72:
		case (char)0x73:
		case (char)0x74:
		case (char)0x75:
		case (char)0x76:
		case (char)0x77:
		case (char)0x78:
		case (char)0x79:
		case (char)0x7a:
		case (char)0x7b:
		case (char)0x7c:
		case (char)0x7d:
		case (char)0x7e:
		case (char)0x7f:
		{
			// two byte JCC -> 6byte JCC
			required_size=6;
			break;
		}

		case (char)0xeb:
		{
			// two byte JMP -> 5byte JMP
			required_size=5;
			break;
		}

		case (char)0xe0:
		case (char)0xe1:
		case (char)0xe2:
		case (char)0xe3:
		{
			// loop, loopne, loopeq, jecxz
			// convert to:
			// <op> +5:
			// jmp fallthrough
			// +5: jmp target
			// 2+5+5;
			required_size=10;
			break;
		}
		

		default:
		{
			required_size=insn->GetDataBits().size();
			if (insn->GetCallback()!="") required_size+=CALLBACK_TRAMPOLINE_SIZE;
			break;
		}
	}
	
	// add an extra 5 for a "trampoline" in case we have to end this fragment early
	return required_size+5;
}

void Zipr_t::ProcessUnpinnedInstruction(const UnresolvedUnpinned_t &uu, const Patch_t &p)
{
	int req_size=DetermineWorstCaseInsnSize(uu.GetInstruction());
	Range_t r=memory_space.GetFreeRange(req_size);
	int insn_count=0;
	const char* truncated="not truncated.";

	RangeAddress_t fr_end=r.GetEnd();
	RangeAddress_t fr_start=r.GetStart();
	RangeAddress_t cur_addr=r.GetStart();
	Instruction_t* cur_insn=uu.GetInstruction();

	if(m_opts.GetVerbose())
		printf("Starting dollop with free range %p-%p\n", (void*)cur_addr, (void*)fr_end);



	/* while we still have an instruction, and we can fit that instruction,
	 * plus a jump into the current free section, plop down instructions sequentially.
	 */
	while(cur_insn && fr_end>(cur_addr+DetermineWorstCaseInsnSize(cur_insn)))
	{
		// some useful bits about how we might do real optimization for pinned instructions.
		DISASM d;
		cur_insn->Disassemble(d);
		int id=cur_insn->GetBaseID();
		RangeAddress_t to_addr;
		/*
		 * Check to see if id is already plopped somewhere.
		 * If so, emit a jump to it and break.
		 * TODO: Test and enable.
		 */
		if (false)
		//if ((to_addr=final_insn_locations[to_insn]) != 0)
		{
			if(m_opts.GetVerbose())
				printf("Fallthrough loop detected. "
				"Emitting jump from %p to %p.\n",
				(void*)cur_addr,
				(void*)to_addr);
			PlopJump(cur_addr);
			PatchJump(cur_addr, to_addr);
			cur_insn = NULL;
			cur_addr+=5;
			break;
		}
		else
		{
			if(m_opts.GetVerbose())
				printf("Emitting %d:%s at %p until ", id, d.CompleteInstr, (void*)cur_addr);
			cur_addr=PlopInstruction(cur_insn,cur_addr);
			if(m_opts.GetVerbose())
				printf("%p\n", (void*)cur_addr);
			cur_insn=cur_insn->GetFallthrough();
			insn_count++;
		}
	}
	if(cur_insn)
	{
		// Mark this insn as needing a patch since we couldn't completely empty 
		// the 'fragment' we are translating into the elf section.
		UnresolvedUnpinned_t uu(cur_insn);
		Patch_t thepatch(cur_addr,UncondJump_rel32);
		patch_list.insert(pair<UnresolvedUnpinned_t,Patch_t>(uu,thepatch));
		PlopJump(cur_addr);
		truncated="truncated due to lack of space.";
		m_stats->total_tramp_space+=5;
		m_stats->truncated_dollops++;
		m_stats->total_trampolines++;
	}

	m_stats->total_dollops++;
	m_stats->total_dollop_instructions+=insn_count;
	m_stats->total_dollop_space+=(cur_addr-fr_start);

	if(m_opts.GetVerbose())
		printf("Ending dollop.  size=%d, %s.  space_remaining=%lld, req'd=%d\n", insn_count, truncated,
		(long long)(fr_end-cur_addr), cur_insn ? DetermineWorstCaseInsnSize(cur_insn) : -1 );
}

void Zipr_t::PlopTheUnpinnedInstructions()
{

	// note that processing a patch may result in adding a new patch
	while(!patch_list.empty())
	{
		// grab first item
		UnresolvedUnpinned_t uu=(*patch_list.begin()).first;
		Patch_t p=(*patch_list.begin()).second;

		DISASM d;
		uu.GetInstruction()->Disassemble(d);
		int id=uu.GetInstruction()->GetBaseID();
		RangeAddress_t at=p.GetAddress();
		
		if(m_opts.GetVerbose())
			printf("Processing patch from %d:%s@%p\n",id,d.CompleteInstr,(void*)at);

		// process the instruction.	
		ProcessUnpinnedInstruction(uu,p); // plopping an instruction results in erasing any patches for it.

		// so erasing it is not necessary
		// patch_list.erase(patch_list.begin());


		

	}
}

void Zipr_t::ApplyPatches(Instruction_t* to_insn)
{

	// insn has been pinned to addr.
	RangeAddress_t to_addr=final_insn_locations[to_insn];
	assert(to_addr!=0);


	// find any patches that require the actual address for instruction insn, 
	// and apply them such that they go to addr.

	UnresolvedUnpinned_t uu(to_insn);

	pair<
		multimap<UnresolvedUnpinned_t,Patch_t>::iterator,
		multimap<UnresolvedUnpinned_t,Patch_t>::iterator
	    > p=patch_list.equal_range(uu);

	for( multimap<UnresolvedUnpinned_t,Patch_t>::iterator mit=p.first;
		mit!=p.second;
		++mit
	   )
	{
		UnresolvedUnpinned_t uu=mit->first;
		assert(uu.GetInstruction()==to_insn);

		DISASM d;
		uu.GetInstruction()->Disassemble(d);
		int id=uu.GetInstruction()->GetBaseID();

		Patch_t p=mit->second;
		RangeAddress_t from_addr=p.GetAddress();

		if(m_opts.GetVerbose())
			printf("Found  a patch for  %p -> %p (%d:%s)\n", 
			(void*)from_addr, (void*)to_addr, id,d.CompleteInstr);
		// Patch instruction
		//
		ApplyPatch(from_addr, to_addr);
	}

	// removing resolved patches
	patch_list.erase(p.first, p.second);
}

void Zipr_t::PatchInstruction(RangeAddress_t from_addr, Instruction_t* to_insn)
{

	// addr needs to go to insn, but insn has not yet been been pinned.


	// patch the instruction at address  addr to go to insn.  if insn does not yet have a concrete address,
	// register that it's patch needs to be applied later. 

	UnresolvedUnpinned_t uu(to_insn);
	Patch_t	thepatch(from_addr,UncondJump_rel32);

	std::map<Instruction_t*,RangeAddress_t>::iterator it=final_insn_locations.find(to_insn);
	if(it==final_insn_locations.end())
	{
		if(m_opts.GetVerbose())
			printf("Instruction cannot be patch yet, as target is unknown.\n");

		patch_list.insert(pair<UnresolvedUnpinned_t,Patch_t>(uu,thepatch));
	}
	else
	{
		RangeAddress_t to_addr=final_insn_locations[to_insn];
		assert(to_addr!=0);
		if(m_opts.GetVerbose())
			printf("Found a patch for %p -> %p\n", (void*)from_addr, (void*)to_addr); 
		// Apply Patch
		ApplyPatch(from_addr, to_addr);
	}
}

#define IS_RELATIVE(A) \
((A.ArgType & MEMORY_TYPE) && (A.ArgType & RELATIVE_))

RangeAddress_t Zipr_t::PlopInstruction(Instruction_t* insn,RangeAddress_t addr)
{
	DISASM d;
	insn->Disassemble(d);
	RangeAddress_t ret=addr;
	bool is_instr_relative = false;


	final_insn_locations[insn]=addr;

	is_instr_relative = IS_RELATIVE(d.Argument1) ||
	                    IS_RELATIVE(d.Argument2) ||
											IS_RELATIVE(d.Argument3);
	if (is_instr_relative) {
		ARGTYPE *relative_arg = NULL;
		uint32_t abs_displacement;
		uint32_t *displacement;
		char instr_raw[20] = {0,};
		int size;
		int offset;
		string raw_data;

		raw_data = insn->GetDataBits();
		assert(raw_data.length() <= 20);

		/*
		 * Which argument is relative? There must be one.
		 */
		if (IS_RELATIVE(d.Argument1)) relative_arg = &d.Argument1;
		if (IS_RELATIVE(d.Argument2)) relative_arg = &d.Argument2;
		if (IS_RELATIVE(d.Argument3)) relative_arg = &d.Argument3;
		assert(relative_arg);

		/*
		 * Calculate the offset into the instruction
		 * of the displacement address.
		 */
		offset = relative_arg->Memory.DisplacementAddr - d.EIP;

		/*
		 * The size of the displacement address must be
		 * four at this point.
		 */
		size = relative_arg->Memory.DisplacementSize;
		assert(size == 4);

		/*
		 * Copy the instruction raw bytes to a place
		 * where we can modify them.
		 */
		memcpy(instr_raw,raw_data.c_str(),raw_data.length());

		/*
		 * Calculate absolute displacement and relative
		 * displacement.
		 */
		displacement = (uint32_t*)(&instr_raw[offset]);
		abs_displacement = *displacement;
		*displacement = abs_displacement - addr;

		printf("absolute displacement: 0x%x\n", abs_displacement);
		printf("relative displacement: 0x%x\n", *displacement);

		/*
		 * Update the instruction with the relative displacement.
		 */
		raw_data.replace(0, raw_data.length(), instr_raw, raw_data.length());
		insn->SetDataBits(raw_data);
	}

	if(insn->GetTarget())
	{
		ret=PlopWithTarget(insn,addr);
	}
	else if(insn->GetCallback()!="")
	{
		ret=PlopWithCallback(insn,addr);
	}
	else
	{
		PlopBytes(addr,insn->GetDataBits().c_str(), insn->GetDataBits().length());
		ret+=insn->GetDataBits().length();
	}

	// now that the insn is put down, adjust any patches that go here 
	ApplyPatches(insn);

	return ret;
}


RangeAddress_t Zipr_t::PlopWithTarget(Instruction_t* insn, RangeAddress_t at)
{
	RangeAddress_t ret=at;
	if(insn->GetDataBits().length() >2) 
	{
		PlopBytes(ret,insn->GetDataBits().c_str(), insn->GetDataBits().length());
		PatchInstruction(ret, insn->GetTarget());	
		ret+=insn->GetDataBits().length();
		return ret;
	}

	// call, jmp, jcc of length 2.
	char b=insn->GetDataBits()[0];
	switch(b)
	{
		case (char)0x70:
		case (char)0x71:
		case (char)0x72:
		case (char)0x73:
		case (char)0x74:
		case (char)0x75:
		case (char)0x76:
		case (char)0x77:
		case (char)0x78:
		case (char)0x79:
		case (char)0x7a:
		case (char)0x7b:
		case (char)0x7c:
		case (char)0x7d:
		case (char)0x7e:
		case (char)0x7f:
		{
		// two byte JCC
			char bytes[]={(char)0x0f,(char)0xc0,(char)0x0,(char)0x0,(char)0x0,(char)0x0 }; 	// 0xc0 is a placeholder, overwritten next statement
			bytes[1]=insn->GetDataBits()[0]+0x10;		// convert to jcc with 4-byte offset.
			PlopBytes(ret,bytes, sizeof(bytes));
			PatchInstruction(ret, insn->GetTarget());	
			ret+=sizeof(bytes);
			return ret;
		}

		case (char)0xeb:
		{
			// two byte JMP
			char bytes[]={(char)0xe9,(char)0x0,(char)0x0,(char)0x0,(char)0x0 }; 	
			bytes[1]=insn->GetDataBits()[0]+0x10;		// convert to jcc with 4-byte offset.
			PlopBytes(ret,bytes, sizeof(bytes));
			PatchInstruction(ret, insn->GetTarget());	
			ret+=sizeof(bytes);
			return ret;
		}

		case (char)0xe0:
		case (char)0xe1:
		case (char)0xe2:
		case (char)0xe3:
		{
			// loop, loopne, loopeq, jecxz
			// convert to:
			// <op> +5:
			// jmp fallthrough
			// +5: jmp target
			char bytes[]={0,0x5};
			bytes[0]=insn->GetDataBits()[0];		
			PlopBytes(ret,bytes, sizeof(bytes));
			ret+=sizeof(bytes);

			PlopJump(ret);
			PatchInstruction(ret, insn->GetFallthrough());	
			ret+=5;

			PlopJump(ret);
			PatchInstruction(ret, insn->GetTarget());	
			ret+=5;
	
			return ret;
			
		}
		

		default:
			assert(0);
	}
}


void Zipr_t::RewritePCRelOffset(RangeAddress_t from_addr,RangeAddress_t to_addr, int insn_length, int offset_pos)
{
	int new_offset=to_addr-from_addr-insn_length;

	byte_map[from_addr+offset_pos+0]=(new_offset>>0)&0xff;
	byte_map[from_addr+offset_pos+1]=(new_offset>>8)&0xff;
	byte_map[from_addr+offset_pos+2]=(new_offset>>16)&0xff;
	byte_map[from_addr+offset_pos+3]=(new_offset>>24)&0xff;
}

void Zipr_t::ApplyPatch(RangeAddress_t from_addr, RangeAddress_t to_addr)
{
	char insn_first_byte=byte_map[from_addr];
	char insn_second_byte=byte_map[from_addr+1];

	switch(insn_first_byte)
	{
		case (char)0xF: // two byte escape
		{
			assert( insn_second_byte==(char)0x80 ||	// should be a JCC 
				insn_second_byte==(char)0x81 ||
				insn_second_byte==(char)0x82 ||
				insn_second_byte==(char)0x83 ||
				insn_second_byte==(char)0x84 ||
				insn_second_byte==(char)0x85 ||
				insn_second_byte==(char)0x86 ||
				insn_second_byte==(char)0x87 ||
				insn_second_byte==(char)0x88 ||
				insn_second_byte==(char)0x89 ||
				insn_second_byte==(char)0x8a ||
				insn_second_byte==(char)0x8b ||
				insn_second_byte==(char)0x8c ||
				insn_second_byte==(char)0x8d ||
				insn_second_byte==(char)0x8e ||
				insn_second_byte==(char)0x8f );

			RewritePCRelOffset(from_addr,to_addr,6,2);
			break;
		}

		case (char)0xe8:	// call
		case (char)0xe9:	// jmp
		{
			RewritePCRelOffset(from_addr,to_addr,5,1);
			break;
		}

		default:
			assert(0);
	}
}


void Zipr_t::FillSection(section* sec, FILE* fexe, section* next_sec)
{
	RangeAddress_t start=sec->get_address();
	RangeAddress_t end=sec->get_size()+start;

	if(next_sec)
	{
		end=extend_section(sec,next_sec);
	}


	if(m_opts.GetVerbose())
		printf("Dumping addrs %p-%p\n", (void*)start, (void*)end);
	for(RangeAddress_t i=start;i<end;i++)
	{
		if(!memory_space.IsByteFree(i))
		{
			// get byte and write it into exe.
			char  b=byte_map[i];
			int file_off=sec->get_offset()+i-start;
			fseek(fexe, file_off, SEEK_SET);
			fwrite(&b,1,1,fexe);
			if(i-start<200)// keep verbose output short enough.
			{
				if(m_opts.GetVerbose())
					printf("Writing byte %#2x at %p, fileoffset=%d\n", 
						((unsigned)b)&0xff, (void*)i, file_off);
			}
		}
	}
}

void Zipr_t::OutputBinaryFile(const string &name)
{
	assert(elfiop);
//	elfiop->load(name);
//	ELFIO::dump::section_headers(cout,*elfiop);

	string myfn=name;
#ifdef CGC
	if(!use_stratafier_mode)
		myfn+=".stripped";
#endif

	printf("Opening %s\n", myfn.c_str());
	FILE* fexe=fopen(myfn.c_str(),"r+");
	assert(fexe);

        // For all sections
        ELFIO::Elf_Half n = elfiop->sections.size();
        for ( ELFIO::Elf_Half i = 0; i < n; ++i )
        {
                section* sec = elfiop->sections[i];
                assert(sec);

                if( (sec->get_flags() & SHF_ALLOC) == 0 )
                        continue;
                if( (sec->get_flags() & SHF_EXECINSTR) == 0)
                        continue;
	

		section* next_sec = NULL;
		if(i+1<n)
			next_sec=elfiop->sections[i+1];

		FillSection(sec, fexe, next_sec);
        }
	fclose(fexe);

	string tmpname=name+string(".to_insert");
	printf("Opening %s\n", tmpname.c_str());
	FILE* to_insert=fopen(tmpname.c_str(),"w");
	string tmpname3=tmpname+"3";	

	if(!to_insert)
		perror( "void Zipr_t::OutputBinaryFile(const string &name)");

	// first byte of this range is the last used byte.
	std::set<Range_t>::iterator it=memory_space.FindFreeRange((RangeAddress_t) -1);
	assert(memory_space.IsValidRange(it));

	RangeAddress_t end_of_new_space=it->GetStart();

	printf("Dumping addrs %p-%p\n", (void*)start_of_new_space, (void*)end_of_new_space);
	for(RangeAddress_t i=start_of_new_space;i<end_of_new_space;i++)
	{
		char b=0;
		if(!memory_space.IsByteFree(i))
		{
			b=byte_map[i];
		}
		if(i-start_of_new_space<200)// keep verbose output short enough.
		{
			if(m_opts.GetVerbose())
				printf("Writing byte %#2x at %p, fileoffset=%lld\n", ((unsigned)b)&0xff, 
				(void*)i, (long long)(i-start_of_new_space));
		}
		fwrite(&b,1,1,to_insert);
	}
	fclose(to_insert);

	AddCallbacksToNewSegment(tmpname,end_of_new_space);
	InsertNewSegmentIntoExe(name,tmpname3,start_of_new_space);
}


void Zipr_t::PrintStats()
{
	m_stats->PrintStats(m_opts, cout);
}

template < typename T > std::string to_hex_string( const T& n )
{
	std::ostringstream stm ;
	stm << std::hex<< "0x"<< n ;
	return stm.str() ;
}

template < typename T > std::string to_string( const T& n )
{
	std::ostringstream stm ;
	stm << n ;
	return stm.str() ;
}


int find_magic_segment_index(ELFIO::elfio *elfiop)
{
        ELFIO::Elf_Half n = elfiop->segments.size();
	ELFIO::Elf_Half i=0;
	ELFIO::segment* last_seg=NULL;
	int last_seg_index=-1;
        for ( i = 0; i < n; ++i )
        {
                ELFIO::segment* seg = elfiop->segments[i];
                assert(seg);
#if 0
                if( (seg->get_flags() & PF_X) == 0 )
                        continue;
                if( (seg->get_flags() & PF_R) == 0)
                        continue;
		last_seg=seg;
		last_seg_index=i;
		break;
#else
		if(seg->get_type() != PT_LOAD)
			continue;
		if(last_seg && (last_seg->get_virtual_address() + last_seg->get_memory_size()) > (seg->get_virtual_address() + seg->get_memory_size())) 
			continue;
		if(seg->get_file_size()==0)
			continue;
		last_seg=seg;
		last_seg_index=i;
#endif
        }
	cout<<"Found magic Seg #"<<std::dec<<last_seg_index<<" has file offset "<<last_seg->get_file_offset()<<endl;
	return last_seg_index;
}

void Zipr_t::InsertNewSegmentIntoExe(string rewritten_file, string bin_to_add, RangeAddress_t sec_start)
{

// from stratafy.pl
//       #system("objcopy  --add-section .strata=strata.linked.data.$$ --change-section-address .strata=$maxaddr --set-section-flags .strata=alloc --set-start $textoffset $exe_copy $newfile") == 0 or die ("command failed $? \n") ;

//        system("$stratafier/add_strata_segment $newfile $exe_copy ") == 0 or die (" command failed : $? \n");

	string cmd="";

	if(use_stratafier_mode)
	{
		cmd= m_opts.GetObjcopyPath() + string(" --add-section .strata=")+bin_to_add+" "+
			string("--change-section-address .strata=")+to_string(sec_start)+" "+
			string("--set-section-flags .strata=alloc,code ")+" "+
			// --set-start $textoffset // set-start not needed, as we aren't changing the entry point.
			rewritten_file;  // editing file in place, no $newfile needed. 
	
		printf("Attempting: %s\n", cmd.c_str());
		if(-1 == system(cmd.c_str()))
		{
			perror(__FUNCTION__);
		}
	
		cmd="$STRATAFIER/add_strata_segment";
		if (m_opts.GetArchitecture() == 64) {
			cmd += "64";
		}
		cmd += " " + rewritten_file+ " " + rewritten_file +".addseg";
		printf("Attempting: %s\n", cmd.c_str());
		if(-1 == system(cmd.c_str()))
		{
			perror(__FUNCTION__);
		}
	
	}
	else
	{
#ifndef CGC
		assert(0); // stratafier mode available only for CGC
#else
		string zeroes_file=rewritten_file+".zeroes";
		cout<<"Note: bss_needed=="<<std::dec<<bss_needed<<endl;
		cmd="cat /dev/zero | head -c "+to_string(bss_needed)+" > "+zeroes_file;
        	printf("Attempting: %s\n", cmd.c_str());
        	if(-1 == system(cmd.c_str()))
        	{
                	perror(__FUNCTION__);
		}
	
        	cmd="cat "+rewritten_file+".stripped "+zeroes_file+" "+bin_to_add+" > "+rewritten_file+".addseg";
        	printf("Attempting: %s\n", cmd.c_str());
        	if(-1 == system(cmd.c_str()))
        	{
                	perror(__FUNCTION__);
        	}
	
        	std::ifstream::pos_type orig_size=filesize((rewritten_file+".stripped").c_str());
        	std::ifstream::pos_type incr_size=bss_needed+filesize(bin_to_add.c_str());
	
        	assert(orig_size+incr_size=filesize((rewritten_file+".addseg").c_str()));
		std::ifstream::pos_type  total_size=orig_size+incr_size;
	
        	ELFIO::elfio *boutaddseg=new ELFIO::elfio;
        	boutaddseg->load(rewritten_file+".addseg");
        	ELFIO::dump::header(cout,*boutaddseg);
        	ELFIO::dump::segment_headers(cout,*boutaddseg);
	
		cout<<"Segments offset is "<<boutaddseg->get_segments_offset()<<endl;
		
	
		ELFIO::Elf_Half i=find_magic_segment_index(boutaddseg);
	
	
		FILE* fboutaddseg=fopen((rewritten_file+".addseg").c_str(),"r+");
		assert(fboutaddseg);
	
		ELFIO::Elf32_Phdr myphdr;
	
		int file_off=boutaddseg->get_segments_offset()+sizeof(ELFIO::Elf32_Phdr)*(i);
	
	cout<<"Seeking to "<<std::hex<<file_off<<endl;
		fseek(fboutaddseg, file_off, SEEK_SET);
		fread(&myphdr, sizeof(myphdr), 1, fboutaddseg);
	cout<<"My phdr has vaddr="<<std::hex<<myphdr.p_vaddr<<endl;
	cout<<"My phdr has phys addr="<<std::hex<<myphdr.p_paddr<<endl;
	cout<<"My phdr has file size="<<std::hex<<myphdr.p_filesz<<endl;
		myphdr.p_filesz=(int)((int)total_size-(int)myphdr.p_offset);
		myphdr.p_memsz=myphdr.p_filesz;
		myphdr.p_flags|=PF_X|PF_R|PF_W;
	cout<<"Updated file size="<<std::hex<<myphdr.p_filesz<<endl;
		fseek(fboutaddseg, file_off, SEEK_SET);
		fwrite(&myphdr, sizeof(myphdr), 1, fboutaddseg);
		fclose(fboutaddseg);
		
		
		ELFIO::Elf32_Shdr myseg_header;
#endif	// #else from #ifndef CGC
	}

	cmd=string("chmod +x ")+rewritten_file+".addseg";
	printf("Attempting: %s\n", cmd.c_str());
	if(-1 == system(cmd.c_str()))
	{
		perror(__FUNCTION__);
	}

}


/*
FIXME
*/
static RangeAddress_t GetCallbackStartAddr()
{
	// add option later, or write code to fix this
	const RangeAddress_t callback_start_addr=0x8048000;
return 0;
	return callback_start_addr;
}


void Zipr_t::AddCallbacksToNewSegment(const string& tmpname, RangeAddress_t end_of_new_space)
{
	const RangeAddress_t callback_start_addr=GetCallbackStartAddr();

	if(m_opts.GetCallbackFileName() == "" )
		return;
	string tmpname2=tmpname+"2";	
	string tmpname3=tmpname+"3";	
	printf("Setting strata library at: %p\n", (void*)end_of_new_space);
	printf("Strata symbols are at %p+addr(symbol)\n", (void*)(end_of_new_space-callback_start_addr));
	string cmd= string("$STRATAFIER/strata_to_data ")+
		m_opts.GetCallbackFileName()+string(" ")+tmpname2+" "+to_hex_string(callback_start_addr);
	printf("Attempting: %s\n", cmd.c_str());
	if(-1 == system(cmd.c_str()))
	{
		perror(__FUNCTION__);
	}

	cmd="cat "+tmpname+" "+tmpname2+" > "+tmpname3;
	printf("Attempting: %s\n", cmd.c_str());
	if(-1 == system(cmd.c_str()))
	{
		perror(__FUNCTION__);
	}
}

RangeAddress_t Zipr_t::PlopWithCallback(Instruction_t* insn, RangeAddress_t at)
{
	RangeAddress_t originalAt = at;
	// emit call <callback>
	{
	char bytes[]={(char)0xe8,(char)0,(char)0,(char)0,(char)0}; // call rel32
	PlopBytes(at, bytes, sizeof(bytes)); 
	unpatched_callbacks.insert(pair<Instruction_t*,RangeAddress_t>(insn,at));
	at+=sizeof(bytes);
	}

	// pop bogus ret addr
	{
	char bytes[]={(char)0x8d,(char)0x64,(char)0x24,(char)m_firp->GetArchitectureBitWidth()/0x08}; // lea esp, [esp+4]
	PlopBytes(at, bytes, sizeof(bytes)); 
	at+=sizeof(bytes);
	}

	assert(CALLBACK_TRAMPOLINE_SIZE<=(at-originalAt));
	return at;
}


// horrible code, rewrite in C++ please!
static RangeAddress_t getSymbolAddress(const string &symbolFilename, const string &symbol) throw(exception)
{
        string symbolFullName = symbolFilename + "+" + symbol;

// nm -a stratafier.o.exe | egrep " integer_overflow_detector$" | cut -f1 -d' '
        string command = "nm -a " + symbolFilename + " | egrep \" " + symbol + "$\" | cut -f1 -d' '";
        char address[1024]="";

	cerr<<"Attempting: "<<command<<endl;

        FILE *fp = popen(command.c_str(), "r");

        fscanf(fp,"%s", address);
	cerr<<"Looking for "<<symbol<<".  Address string is "<<address<<endl;
        string addressString = string(address);
        pclose(fp);

        RangeAddress_t ret= (uintptr_t) strtoull(addressString.c_str(),NULL,16);

        //TODO: throw exception if address is not found.
        //for now assert the address string isn't empty
        if(addressString.empty())
        {
                cerr<<"Cannot find symbol "<< symbol << " in " << symbolFilename << "."<<endl;
		addressString="0x0";
		return 0;
        }
	else
	{
		cerr<<"Found symbol "<< symbol << " in " << symbolFilename << " at " << std::hex << ret << "."<<endl;
		return ret;
	}

}


RangeAddress_t Zipr_t::FindCallbackAddress(RangeAddress_t end_of_new_space, RangeAddress_t start_addr, const string &callback)
{
	if(callback_addrs.find(callback)==callback_addrs.end())
	{

		RangeAddress_t addr=getSymbolAddress(m_opts.GetCallbackFileName(),callback);

		if(addr!=0)
		{
			/* adjust by start of new location, - beginning of old location */
			addr=addr+end_of_new_space-start_addr;
		}
		cout<<" Addr adjusted to "<<std::hex<<addr<<endl;
		callback_addrs[callback]=addr;
	}
	return callback_addrs[callback];

}

void Zipr_t::UpdateCallbacks()
{
        // first byte of this range is the last used byte.
        set<Range_t>::iterator it=memory_space.FindFreeRange((RangeAddress_t) -1);
        assert(memory_space.IsValidRange(it));

        RangeAddress_t end_of_new_space=it->GetStart();
	RangeAddress_t start_addr=GetCallbackStartAddr();

	for( std::set<std::pair<libIRDB::Instruction_t*,RangeAddress_t> >::iterator it=unpatched_callbacks.begin();
		it!=unpatched_callbacks.end();
		++it
	   )
	{
		Instruction_t *insn=it->first;
		RangeAddress_t at=it->second;
		RangeAddress_t to=FindCallbackAddress(end_of_new_space,start_addr,insn->GetCallback());
		if(to)
		{
			cout<<"Patching callback "<< insn->GetCallback()<<"at "<<std::hex<<at<<" to jump to "<<to<<endl;
			PatchCall(at,to);
		}
		else
			CallToNop(at);
	}
}
