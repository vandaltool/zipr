#include <zipr_all.h>
#include <libIRDB-core.hpp>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <assert.h>
#include <sys/mman.h>
#include <ctype.h>

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

	// resolve unpinned, otherwise unresolved instructions
	// truncate final range.
	// write binary file to disk 
}

void Zipr_t::FindFreeRanges(const std::string &name)
{
	/* use ELFIO to load the sections */
	ELFIO::elfio*    elfiop=new ELFIO::elfio;
	assert(elfiop);
	elfiop->load(name);
//	ELFIO::dump::header(cout,*elfiop);
	ELFIO::dump::section_headers(cout,*elfiop);

	RangeAddress_t last_end=0;

	// For all sections
	Elf_Half n = elfiop->sections.size();
	for ( Elf_Half i = 0; i < n; ++i ) 
	{ 
		section* sec = elfiop->sections[i];
		assert(sec);

		RangeAddress_t start=sec->get_address();
		RangeAddress_t end=sec->get_size()+start-1;

		if( (sec->get_flags() & SHF_ALLOC) ==0 )
			continue;

		assert(start>last_end);
		last_end=end;

		if((sec->get_flags() & SHF_EXECINSTR))
		{
			printf("Adding free range 0x%p to 0x%p\n", (void*)start,(void*)end);
			free_ranges.push_back(Range_t(start,end));
		}
	}

#define PAGE_SIZE 4096
#define PAGE_ROUND_UP(x) ( (((uintptr_t)(x)) + PAGE_SIZE-1)  & (~(PAGE_SIZE-1)) )

	// now that we've looked at the sections, add a (mysterious) extra section in case we need to overflow 
	// the sections existing in the ELF.
	RangeAddress_t new_free_page=PAGE_ROUND_UP(last_end);
	free_ranges.push_back( Range_t(new_free_page,(RangeAddress_t)-1));
	printf("Adding (mysterious) free range 0x%p to EOF\n", (void*)new_free_page);
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
		RangeAddress_t addr=upinsn->GetIndirectBranchTargetAddress()->GetVirtualOffset();

		if(upinsn->GetAddress()->GetFileID()==BaseObj_t::NOT_IN_DATABASE)
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
		}
		else
		{
                	++it;
			printf("Found %p can NOT be updated to 5-byte jmp\n", (void*)addr);
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

				PatchJump(addr, addr-i);
				five_byte_pins[up]=addr-i;
				PlopJump(addr-i);
				break;
			}
			else if(AreBytesFree(addr+i,5))
			{
				printf("Found location for 2-byte->5-byte conversion (%p-%p)->(%p-%p)\n", 
					(void*)addr,(void*)(addr+1), (void*)(addr+i),(void*)(addr+i+5)); 
				five_byte_pins[up]=addr+i;
				PlopJump(addr+i);
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

		printf("Converting 5-byte pinned jump to patch for %p-%p\n", (void*)addr,(void*)(addr+4));

		// remove and move to next pin
		five_byte_pins.erase(it++);
	}
		
#if 0
// some useful bits about how we might do real optimization for pinned instructions.
		printf("Found %p can be updated to use better than 2-byte jmp\n", (void*)addr);
	
		list<Range_t>::iterator it=FindFreeRange(addr+2);
		assert(it!=free_ranges.end());
		Range_t r=*it;
		RangeAddress_t fr_end=r->GetEnd();
		RangeAddress_t cur_addr=addr;
		Instruction_t* cur_insn=up->GetInstruction();

		/* while we still have an instruction, and we can fit that instruction,
		 * plus a jump into the current free section, plop down instructions sequentially.
		 */
		while(cur_insn && fr_end>(cur_addr+cur_insn->GetDataBits().length()+5))
		{
			cur_addr+=PlopInstruction(cur_insn,addr);
			cur_insn=cur_insn->GetFallthrough();
		}

		if(cur_insn)
			PlopJump(addr, cur_insn);
#endif
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
			required_size=insn->GetDataBits().size()+5;
			break;
		}
	}
	return required_size;
}

void Zipr_t::ProcessUnpinnedInstruction(const UnresolvedUnpinned_t &uu, const Patch_t &p)
{
	int req_size=DetermineWorseCaseInsnSize(uu.GetInstruction());
	Range_t r=GetFreeRange(req_size);




	RangeAddress_t fr_end=r.GetEnd();
	RangeAddress_t cur_addr=r.GetStart();
	Instruction_t* cur_insn=uu.GetInstruction();



	/* while we still have an instruction, and we can fit that instruction,
	 * plus a jump into the current free section, plop down instructions sequentially.
	 */
	while(cur_insn && fr_end>(cur_addr+DetermineWorseCaseInsnSize(cur_insn)))
	{
		// some useful bits about how we might do real optimization for pinned instructions.
		DISASM d;
		cur_insn->Disassemble(d);
		printf("Emitting %s at %p\n", d.CompleteInstr, (void*)cur_addr);


		cur_addr=PlopInstruction(cur_insn,cur_addr);
		cur_insn=cur_insn->GetFallthrough();
	}
	if(cur_insn)
	{
		// Mark this insn as needing a patch since we couldn't completely empty 
		// the 'fragment' we are translating into the elf section.
                UnresolvedUnpinned_t uu(cur_insn);
                Patch_t thepatch(cur_addr,UncondJump_rel32);
                patch_list.insert(pair<UnresolvedUnpinned_t,Patch_t>(uu,thepatch));
		PlopJump(cur_addr);
	}
}

void Zipr_t::PlopTheUnpinnedInstructions()
{

	// note that processing a patch may result in adding a new patch
	while(!patch_list.empty())
	{
		// grab first item
		UnresolvedUnpinned_t uu=(*patch_list.begin()).first;
		Patch_t p=(*patch_list.begin()).second;

		// and erase it.
		patch_list.erase(patch_list.begin());

		// process the instruction.	
		ProcessUnpinnedInstruction(uu,p);

		

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

		Patch_t p=mit->second;
		RangeAddress_t from_addr=p.GetAddress();

		printf("Found  a patch for  %p -> %p\n", 
			(void*)from_addr, (void*)to_addr);
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
		ApplyPatches(insn);
	}

	return ret;
}


RangeAddress_t Zipr_t::PlopWithTarget(Instruction_t* insn, RangeAddress_t at)
{
	RangeAddress_t ret=at;
	if(insn->GetDataBits().length() >2) 
	{
		PlopBytes(ret,insn->GetDataBits().c_str(), insn->GetDataBits().length());
		ret+=insn->GetDataBits().length();
		PatchInstruction(ret, insn->GetTarget());	
		return ret;
	}

	// call, jmp, jcc of length 2.
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
			assert( insn_second_byte==0x80 ||	// should be a JCC 
				insn_second_byte==0x81 ||
				insn_second_byte==0x82 ||
				insn_second_byte==0x83 ||
				insn_second_byte==0x84 ||
				insn_second_byte==0x85 ||
				insn_second_byte==0x86 ||
				insn_second_byte==0x87 ||
				insn_second_byte==0x88 ||
				insn_second_byte==0x89 ||
				insn_second_byte==0x8a ||
				insn_second_byte==0x8b ||
				insn_second_byte==0x8c ||
				insn_second_byte==0x8d ||
				insn_second_byte==0x8e ||
				insn_second_byte==0x8f );

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

