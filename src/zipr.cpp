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

#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"
#include "targ-config.h"
#include "beaengine/BeaEngine.h"


using namespace libIRDB;
using namespace std;
using namespace zipr;
using namespace ELFIO;


void Zipr_t::CreateBinaryFile(const std::string &name)
{

	// create ranges, including extra range that's def. big enough.
	FindFreeRanges(name);

	// add pinned instructions
	AddPinnedInstructions();

	// reserve space for pins
	ReservePinnedInstructions();

	// expand 2-byte pins into 4-byte pins
	ExpandPinnedInstructions();

	// Allocate space near 2-byte pins for a 5-byte pin
	Fix2BytePinnedInstructions();

	// Convert all 5-byte pins into full fragments
	OptimizePinnedInstructions();

	// now that pinning is done, start emitting unpinnned instructions, and patching where needed.
	PlopTheUnpinnedInstructions();

	// write binary file to disk 
	OutputBinaryFile(name);

	// print relevant information
	PrintStats();
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

	// For all sections
	Elf_Half n = elfiop->sections.size();
	for ( Elf_Half i = 0; i < n; ++i ) 
	{ 
		section* sec = elfiop->sections[i];
		assert(sec);

		RangeAddress_t start=sec->get_address();
		RangeAddress_t end=sec->get_size()+start-1;

		printf("max_addr is %p, end is %p\n", (void*)max_addr, (void*)end);
		if(start && end>max_addr)
		{
			printf("new max_addr is %p\n", (void*)max_addr);
			max_addr=end;
		}

		if( (sec->get_flags() & SHF_ALLOC) ==0 )
			continue;


		if((sec->get_flags() & SHF_EXECINSTR))
		{
			assert(start>last_end);
			last_end=end;

			printf("Adding free range 0x%p to 0x%p\n", (void*)start,(void*)end);
			free_ranges.push_back(Range_t(start,end));
		}
	}

#define PAGE_SIZE 4096
#define PAGE_ROUND_UP(x) ( (((uintptr_t)(x)) + PAGE_SIZE-1)  & (~(PAGE_SIZE-1)) )

	// now that we've looked at the sections, add a (mysterious) extra section in case we need to overflow 
	// the sections existing in the ELF.
	RangeAddress_t new_free_page=PAGE_ROUND_UP(max_addr);
	free_ranges.push_back( Range_t(new_free_page,(RangeAddress_t)-1));
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


Range_t Zipr_t::GetFreeRange(int size)
{
	for( list<Range_t>::iterator it=free_ranges.begin();
		it!=free_ranges.end();
		++it)
	{
		Range_t r=*it;
		if(r.GetEnd() - r.GetStart() > size)
			return r;
	}
	assert(0);// assume we find a big enough range.
}

list<Range_t>::iterator Zipr_t::FindFreeRange(RangeAddress_t addr)
{
	for( list<Range_t>::iterator it=free_ranges.begin();
		it!=free_ranges.end();
		++it)
	{
		Range_t r=*it;
		if(r.GetStart() <= addr && addr <=r.GetEnd())
			return it;
	}
	return free_ranges.end();
}

void Zipr_t::SplitFreeRange(RangeAddress_t addr)
{
	list<Range_t>::iterator it=FindFreeRange(addr);
	assert(it!=free_ranges.end());

	Range_t r=*it;

	if(r.GetStart()==r.GetEnd())
	{
		assert(addr==r.GetEnd());
		free_ranges.erase(it);
	}
	else if(addr==r.GetStart())
	{
		free_ranges.insert(it, Range_t(r.GetStart()+1, r.GetEnd()));
		free_ranges.erase(it);
	}
	else if(addr==r.GetEnd())
	{
		free_ranges.insert(it, Range_t(r.GetStart(), r.GetEnd()-1));
		free_ranges.erase(it);
	}
	else // split range 
	{
		free_ranges.insert(it, Range_t(r.GetStart(), addr-1));
		free_ranges.insert(it, Range_t(addr+1, r.GetEnd()));
		free_ranges.erase(it);
	}
}


static bool should_pin_immediately(Instruction_t *upinsn)
{
	DISASM d;
	upinsn->Disassemble(d);

	if(d.Instruction.BranchType==RetType)
		return true;

	AddressID_t* upinsn_ibta=upinsn->GetIndirectBranchTargetAddress();
	assert(upinsn_ibta!=NULL);

	/* careful with 1 byte instructions that have a pinned fallthrough */ 
	if(upinsn->GetDataBits().length()==1)
	{
		if(upinsn->GetFallthrough()==NULL)
			return true;
		AddressID_t* ft_ibta=upinsn->GetFallthrough()->GetIndirectBranchTargetAddress();
		if(ft_ibta && (upinsn_ibta->GetVirtualOffset()+1) == ft_ibta->GetVirtualOffset())
			return true;
	}

	return false;
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
		if(should_pin_immediately(upinsn))
		{
			printf("Final pinning %p-%p.  fid=%d\n", (void*)addr, (void*)(addr+upinsn->GetDataBits().size()-1),
				upinsn->GetAddress()->GetFileID());
			for(int i=0;i<upinsn->GetDataBits().size();i++)
			{
				byte_map[addr+i]=upinsn->GetDataBits()[i];
				SplitFreeRange(addr+i);
				total_other_space++;
			}
			continue;
		}

		char bytes[]={0xeb,0}; // jmp rel8
		printf("Two-byte Pinning %p-%p.  fid=%d\n", (void*)addr, (void*)(addr+sizeof(bytes)-1),
				upinsn->GetAddress()->GetFileID());
	
		two_byte_pins.insert(up);
		for(int i=0;i<sizeof(bytes);i++)
		{
			assert(byte_map.find(addr+i) == byte_map.end() );
			byte_map[addr+i]=bytes[i];
			SplitFreeRange(addr+i);
		}
	}

}


void Zipr_t::ExpandPinnedInstructions()
{
	/* now, all insns have 2-byte pins.  See which ones we can make 5-byte pins */
	
        for(   
		set<UnresolvedPinned_t>::const_iterator it=two_byte_pins.begin();
                it!=two_byte_pins.end();
           )
	{
		UnresolvedPinned_t up=*it;
		Instruction_t* upinsn=up.GetInstruction();
		RangeAddress_t addr=upinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset();


		char bytes[]={0xe9,0,0,0,0}; // jmp rel8
		bool can_update=AreBytesFree(addr+2,sizeof(bytes)-2);
		if(can_update)
		{
			printf("Found %p can be updated to 5-byte jmp\n", (void*)addr);
			PlopJump(addr);
			five_byte_pins[up]=addr;
			two_byte_pins.erase(it++);
			total_5byte_pins++;
			total_trampolines++;
		}
		else
		{
                	++it;
			printf("Found %p can NOT be updated to 5-byte jmp\n", (void*)addr);
			total_2byte_pins++;
			total_trampolines++;
			total_tramp_space+=2;
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
		RangeAddress_t addr=upinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset();

		// check for near branch instructions
		for(int i=0;i<120;i++)	// two byte jump range is 127 bytes, but that's from the pc after it's been inc, etc. complicated goo.  120 is a safe estimate of range.
		{
			if(AreBytesFree(addr-i,5))
			{
				printf("Found location for 2-byte->5-byte conversion (%p-%p)->(%p-%p)\n", 
					(void*)addr,(void*)(addr+1), (void*)(addr-i),(void*)(addr-i+4)); 

				five_byte_pins[up]=addr-i;
				PlopJump(addr-i);
				PatchJump(addr, addr-i);
				break;
			}
			else if(AreBytesFree(addr+i,5))
			{
				printf("Found location for 2-byte->5-byte conversion (%p-%p)->(%p-%p)\n", 
					(void*)addr,(void*)(addr+1), (void*)(addr+i),(void*)(addr+i+5)); 

				five_byte_pins[up]=addr+i;
				PlopJump(addr+i);
				PatchJump(addr, addr+i);
				break;
			}
			else
			{
//				printf("Not free at %p or %p\n", (void*)(addr-i),(void*)(addr+i));
			}
		}
		two_byte_pins.erase(it++);
	}

}


bool Zipr_t::AreBytesFree(RangeAddress_t addr, int num_bytes)
{
	for(int i=0;i<num_bytes;i++)
		if(!IsByteFree(addr+i))
			return false;
	return true;
}

bool Zipr_t::IsByteFree(RangeAddress_t addr)
{
	if(FindFreeRange(addr)!=free_ranges.end())
		return true;
	return false;
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
			printf("Converting 5-byte pinned jump at %p-%p to patch to %d:%s\n", 
				(void*)addr,(void*)(addr+4), uu.GetInstruction()->GetBaseID(), d.CompleteInstr);
			total_tramp_space+=5;
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
		SplitFreeRange(addr);
	byte_map[addr]=the_byte;
}

void Zipr_t::PlopJump(RangeAddress_t addr)
{
	char bytes[]={0xe9,0,0,0,0}; // jmp rel8
	for(int i=0;i<sizeof(bytes);i++)        // don't check bytes 0-1, because they've got the short byte jmp
	{
		PlopByte(addr+i,bytes[i]);
	}
}


void Zipr_t::PatchJump(RangeAddress_t at_addr, RangeAddress_t to_addr)
{
	uintptr_t off=to_addr-at_addr-2;

	assert(!IsByteFree(at_addr));
	
	switch(byte_map[at_addr])
	{
		case (char)0xe9:	/* 5byte jump */
		{
			assert(0);
		}
		case (char)0xeb:	/* 2byte jump */
		{
			assert(off==(uintptr_t)(char)off);

			assert(!IsByteFree(at_addr+1));
			byte_map[at_addr+1]=(char)off;
		}
	}
}


static int DetermineWorseCaseInsnSize(Instruction_t* insn)
{

	int required_size=0;

	switch(insn->GetDataBits()[0])
	{
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
			break;
		}
	}
	
	// add an extra 5 for a "trampoline" in case we have to end this fragment early
	return required_size+5;
}

void Zipr_t::ProcessUnpinnedInstruction(const UnresolvedUnpinned_t &uu, const Patch_t &p)
{
	int req_size=DetermineWorseCaseInsnSize(uu.GetInstruction());
	Range_t r=GetFreeRange(req_size);
	int insn_count=0;
	const char* truncated="not truncated.";

	RangeAddress_t fr_end=r.GetEnd();
	RangeAddress_t fr_start=r.GetStart();
	RangeAddress_t cur_addr=r.GetStart();
	Instruction_t* cur_insn=uu.GetInstruction();

	printf("Starting dollop with free range %p-%p\n", (void*)cur_addr, (void*)fr_end);



	/* while we still have an instruction, and we can fit that instruction,
	 * plus a jump into the current free section, plop down instructions sequentially.
	 */
	while(cur_insn && fr_end>(cur_addr+DetermineWorseCaseInsnSize(cur_insn)))
	{
		// some useful bits about how we might do real optimization for pinned instructions.
		DISASM d;
		cur_insn->Disassemble(d);
		int id=cur_insn->GetBaseID();
		printf("Emitting %d:%s at %p\n", id, d.CompleteInstr, (void*)cur_addr);


		cur_addr=PlopInstruction(cur_insn,cur_addr);
		cur_insn=cur_insn->GetFallthrough();
		insn_count++;
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
		total_tramp_space+=5;
		truncated_dollops++;
		total_trampolines++;
	}

	total_dollops++;
	total_dollop_instructions+=insn_count;
	total_dollop_space+=(cur_addr-fr_start);

	printf("Ending dollop.  size=%d, %s.  space_remaining=%lld, req'd=%d\n", insn_count, truncated,
		(long long)(fr_end-cur_addr), cur_insn ? DetermineWorseCaseInsnSize(cur_insn) : -1 );
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
		printf("Instruction cannot be patch yet, as target is unknown.\n");

		patch_list.insert(pair<UnresolvedUnpinned_t,Patch_t>(uu,thepatch));
	}
	else
	{
		RangeAddress_t to_addr=final_insn_locations[to_insn];
		assert(to_addr!=0);
		printf("Found a patch for %p -> %p\n", (void*)from_addr, (void*)to_addr); 
		// Apply Patch
		ApplyPatch(from_addr, to_addr);
	}
}


RangeAddress_t Zipr_t::PlopInstruction(Instruction_t* insn,RangeAddress_t addr)
{
	DISASM d;
	insn->Disassemble(d);
	RangeAddress_t ret=addr;


	final_insn_locations[insn]=addr;

	if(insn->GetTarget())
	{
		ret=PlopWithTarget(insn,addr);
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
			char bytes[]={0x0f,0xc0,0x0,0x0,0x0,0x0 }; 	// 0xc0 is a placeholder, overwritten next statement
			bytes[1]=insn->GetDataBits()[0]+0x10;		// convert to jcc with 4-byte offset.
			PlopBytes(ret,bytes, sizeof(bytes));
			PatchInstruction(ret, insn->GetTarget());	
			ret+=sizeof(bytes);
			return ret;
		}

		case (char)0xeb:
		{
			// two byte JMP
			char bytes[]={0xe9,0x0,0x0,0x0,0x0 }; 	
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


void Zipr_t::FillSection(section* sec, FILE* fexe)
{
	RangeAddress_t start=sec->get_address();
	RangeAddress_t end=sec->get_size()+start;



	printf("Dumping addrs %p-%p\n", (void*)start, (void*)end);
	for(RangeAddress_t i=start;i<end;i++)
	{
		if(!IsByteFree(i))
		{
			// get byte and write it into exe.
			char  b=byte_map[i];
			int file_off=sec->get_offset()+i-start;
			fseek(fexe, file_off, SEEK_SET);
			fwrite(&b,1,1,fexe);
			if(i-start<200)// keep verbose output short enough.
			{
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
	ELFIO::dump::section_headers(cout,*elfiop);

	printf("Opening %s\n", name.c_str());
	FILE* fexe=fopen(name.c_str(),"r+");
	assert(fexe);

        // For all sections
        Elf_Half n = elfiop->sections.size();
        for ( Elf_Half i = 0; i < n; ++i )
        {
                section* sec = elfiop->sections[i];
                assert(sec);

                if( (sec->get_flags() & SHF_ALLOC) == 0 )
                        continue;
                if( (sec->get_flags() & SHF_EXECINSTR) == 0)
                        continue;

		FillSection(sec, fexe);
        }
	fclose(fexe);

	string tmpname=string("to_insert")+name;
	printf("Opening %s\n", tmpname.c_str());
	FILE* to_insert=fopen(tmpname.c_str(),"w");

	if(!to_insert)
		perror( "void Zipr_t::OutputBinaryFile(const string &name)");

	// first byte of this range is the last used byte.
	list<Range_t>::iterator it=FindFreeRange((RangeAddress_t) -1);
	assert(it!=free_ranges.end());

	RangeAddress_t end_of_new_space=it->GetStart();

	printf("Dumping addrs %p-%p\n", (void*)start_of_new_space, (void*)end_of_new_space);
	for(RangeAddress_t i=start_of_new_space;i<end_of_new_space;i++)
	{
		char b=0;
		if(!IsByteFree(i))
		{
			b=byte_map[i];
		}

	
		if(i-start_of_new_space<200)// keep verbose output short enough.
			printf("Writing byte %#2x at %p, fileoffset=%lld\n", ((unsigned)b)&0xff, 
				(void*)i, (long long)(i-start_of_new_space));
		fwrite(&b,1,1,to_insert);
	}
	fclose(to_insert);

	InsertNewSegmentIntoExe(name,tmpname,start_of_new_space);
}


void Zipr_t::PrintStats()
{
	cout<<"Total dollops: "<<std::dec << total_dollops <<endl;
	cout<<"Total dollop size: "<<std::dec << total_dollop_space <<endl;
	cout<<"Total dollop instructions: "<<std::dec << total_dollop_instructions <<endl;
	cout<<"Truncated dollops: "<<std::dec << truncated_dollops <<endl;
	cout<<"Ave dollop size: "<<std::dec << (double)total_dollop_space/(double)total_dollops <<endl;
	cout<<"Ave dollop instructions: "<<std::dec << (double)total_dollop_instructions/(double)total_dollops <<endl;
	cout<<"Truncated dollop fraction: "<<std::dec << (double)truncated_dollops/(double)total_dollops <<endl;

	cout<<"Total trampolines: "<<std::dec << total_trampolines <<endl;
	cout<<"Total 2-byte pin trampolines: "<<std::dec << total_2byte_pins <<endl;
	cout<<"Total 5-byte pin trampolines: "<<std::dec << total_5byte_pins <<endl;
	cout<<"Total trampoline space pins: "<<std::dec << total_tramp_space <<endl;

	cout<<"Other space: "<<total_other_space<<endl;
}

template < typename T > std::string to_string( const T& n )
{
	std::ostringstream stm ;
	stm << n ;
	return stm.str() ;
}


void Zipr_t::InsertNewSegmentIntoExe(string rewritten_file, string bin_to_add, RangeAddress_t sec_start)
{

// from stratafy.pl
//       #system("objcopy  --add-section .strata=strata.linked.data.$$ --change-section-address .strata=$maxaddr --set-section-flags .strata=alloc --set-start $textoffset $exe_copy $newfile") == 0 or die ("command failed $? \n") ;

//        system("$stratafier/add_strata_segment $newfile $exe_copy ") == 0 or die (" command failed : $? \n");

	string  cmd=
		string("objcopy  --add-section .strata=")+bin_to_add+ 
		string(" --change-section-address .strata=")+to_string(sec_start)+
		string(" --set-section-flags .strata=alloc,code ")+ 
		// --set-start $textoffset // set-start not needed, as we aren't changing the entry point.
		rewritten_file;  // editing file in place, no $newfile needed. 

	printf("Attempting: %s\n", cmd.c_str());
	if(-1 == system(cmd.c_str()))
	{
		perror(__FUNCTION__);
	}

	cmd="$STRATAFIER/add_strata_segment "+rewritten_file+ " " + rewritten_file +".addseg";
	printf("Attempting: %s\n", cmd.c_str());
	if(-1 == system(cmd.c_str()))
	{
		perror(__FUNCTION__);
	}

}
