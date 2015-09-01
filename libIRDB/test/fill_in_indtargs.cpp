
/*
 * Copyright (c) 2014 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */

#include <libIRDB-core.hpp>
#include <iostream>
#include <limits>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <regex.h>
// #include <elf.h>
#include <ctype.h>
#include <list>
#include <stdio.h>


#include <exeio.h>
#include "beaengine/BeaEngine.h"
#include "check_thunks.hpp"

using namespace libIRDB;
using namespace std;
using namespace EXEIO;

#define HELLNODE_ID 0
#define INDIRECT_CALLS_ID 1
int next_icfs_set_id = 2;

ICFS_t* hellnode_tgts = NULL;
ICFS_t* indirect_calls = NULL;

#define arch_ptr_bytes() (firp->GetArchitectureBitWidth()/8)

int odd_target_count=0;
int bad_target_count=0;
int bad_fallthrough_count=0;

bool is_possible_target(virtual_offset_t p, virtual_offset_t addr);

set< pair <virtual_offset_t,virtual_offset_t>  > bounds;
set<virtual_offset_t> targets;

set< pair< virtual_offset_t, virtual_offset_t> > ranges;

// a way to map an instruction to its set of predecessors. 
map< Instruction_t* , set<Instruction_t*> > preds;

// keep track of jmp tables
map< Instruction_t*, set<Instruction_t*> > jmptables;

void check_for_PIC_switch_table32_type2(Instruction_t* insn, DISASM disasm, EXEIO::exeio* elfiop, const set<virtual_offset_t>& thunk_bases);
void check_for_PIC_switch_table32_type3(Instruction_t* insn, DISASM disasm, EXEIO::exeio* elfiop, const set<virtual_offset_t>& thunk_bases);
void check_for_PIC_switch_table32(FileIR_t*, Instruction_t* insn, DISASM disasm, EXEIO::exeio* elfiop, const set<virtual_offset_t>& thunk_bases);
void check_for_PIC_switch_table64(FileIR_t*, Instruction_t* insn, DISASM disasm, EXEIO::exeio* elfiop);

// get switch table structure, determine ib targets
// handle both 32 and 64 bit
void check_for_nonPIC_switch_table(FileIR_t*, Instruction_t* insn, DISASM disasm, EXEIO::exeio* elfiop);
void check_for_nonPIC_switch_table_pattern2(FileIR_t*, Instruction_t* insn, DISASM disasm, EXEIO::exeio* elfiop);

void check_for_indirect_jmp(FileIR_t* const firp, Instruction_t* const insn);
void check_for_indirect_call(FileIR_t* const firp, Instruction_t* const insn);
void check_for_ret(FileIR_t* const firp, Instruction_t* const insn);

void range(virtual_offset_t start, virtual_offset_t end)
{ 	
	pair<virtual_offset_t,virtual_offset_t> foo(start,end);
	ranges.insert(foo);
}

bool is_in_range(virtual_offset_t p)
{
	for(
		set< pair <virtual_offset_t,virtual_offset_t>  >::iterator it=ranges.begin();
		it!=ranges.end();
		++it
	   )
	{
		pair<virtual_offset_t,virtual_offset_t> bound=*it;
		virtual_offset_t start=bound.first;
		virtual_offset_t end=bound.second;
		if(start<=p && p<=end)
			return true;
	}
	return false;
}

void process_ranges(FileIR_t* firp)
{
        for(
                set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
                it!=firp->GetInstructions().end();
                ++it
           )
	{
                Instruction_t *insn=*it;
                DISASM disasm;
#if 0
                memset(&disasm, 0, sizeof(DISASM));

                disasm.Options = NasmSyntax + PrefixedNumeral;
                disasm.Archi = 32;
                disasm.EIP = (UIntPtr) insn->GetDataBits().c_str();
                disasm.VirtualAddr = insn->GetAddress()->GetVirtualOffset();
#endif
                int instr_len = insn->Disassemble(disasm);

                assert(instr_len==insn->GetDataBits().size());

                /* calls indicate an indirect target, pc+sizeof(instruction) */
                if(disasm.Instruction.BranchType==CallType)
                {
			if(is_in_range(disasm.VirtualAddr+instr_len))
                        	possible_target(disasm.VirtualAddr+instr_len);
                }
	}
}

bool possible_target(virtual_offset_t p, virtual_offset_t addr)
{
/*	if(p!=(int)p)
	{
		if(getenv("IB_VERBOSE")!=NULL)
			cout<<"Determined "<<hex<<p<<" cannot be a code pointer"<<endl;
		return false;
	}
*/
	if(is_possible_target(p,addr))
	{
		if(getenv("IB_VERBOSE")!=NULL)
		{
			if(addr!=0)
				cout<<"Found IB target address 0x"<<std::hex<<p<<" at 0x"<<addr<<std::dec<<endl;
			else
				cout<<"Found IB target address 0x"<<std::hex<<p<<" from unknown location"<<endl;
		}
		targets.insert(p);
		return true;
	}
	return false;
}

bool is_possible_target(virtual_offset_t p, virtual_offset_t addr)
{
/*
	if(p!=(int)p)
	{
		return false;	// can't be a pointer if it's greater than 2gb. 
	}
*/
	for(
		set< pair <virtual_offset_t,virtual_offset_t>  >::iterator it=bounds.begin();
		it!=bounds.end();
		++it
	   )
	{
		pair<virtual_offset_t,virtual_offset_t> bound=*it;
		virtual_offset_t start=bound.first;
		virtual_offset_t end=bound.second;
		if(start<=p && p<=end)
		{
			return true;
		}
        }
	return false;

}

EXEIO::section*  find_section(virtual_offset_t addr, EXEIO::exeio *elfiop)
{
         for ( int i = 0; i < elfiop->sections.size(); ++i )
         {   
                 EXEIO::section* pSec = elfiop->sections[i];
                 assert(pSec);
                 if(pSec->get_address() > addr)
                         continue;
                 if(addr >= pSec->get_address()+pSec->get_size())
                         continue;

                 return pSec;
	}
	return NULL;
}

void handle_argument(ARGTYPE *arg, Instruction_t* insn)
{
	if( (arg->ArgType&MEMORY_TYPE) == MEMORY_TYPE ) 
	{
		if((arg->ArgType&RELATIVE_)==RELATIVE_)
		{
			assert(insn);
			assert(insn->GetAddress());
			possible_target(arg->Memory.Displacement+insn->GetAddress()->GetVirtualOffset()+
				insn->GetDataBits().length());
		}
		else
			possible_target(arg->Memory.Displacement);
	}
}


static map<virtual_offset_t,Instruction_t*> lookupInstructionMap;
void lookupInstruction_init(FileIR_t *firp)
{
	lookupInstructionMap.clear();
	for(set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end(); ++it)
        {
		Instruction_t *insn=*it;
		virtual_offset_t addr=insn->GetAddress()->GetVirtualOffset();
		lookupInstructionMap[addr]=insn;
	}
}

Instruction_t *lookupInstruction(FileIR_t *firp, virtual_offset_t virtual_offset)
{
	if(lookupInstructionMap.find(virtual_offset)!=lookupInstructionMap.end())
		return lookupInstructionMap[virtual_offset];
	return NULL;
}

void mark_jmptables(FileIR_t *firp)
{
	map< Instruction_t*, set<Instruction_t*> >::iterator it;
	for (it = jmptables.begin(); it != jmptables.end(); ++it)
	{
		Instruction_t* instr = it->first;
		set<Instruction_t*> instruction_targets = it->second;

		assert(instruction_targets.size() > 0);

		ICFS_t* new_icfs = new ICFS_t(next_icfs_set_id++, true);
		*new_icfs = instruction_targets;
		firp->GetAllICFS().insert(new_icfs);

		instr->SetIBTargets(new_icfs);

		if(getenv("IB_VERBOSE")!=0)
			cout << "jmp table[" << new_icfs->GetBaseID() << "]: size: " << new_icfs->size() << endl;
	}
}

void mark_targets(FileIR_t *firp)
{
        for(
                set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
                it!=firp->GetInstructions().end();
                ++it
           )
        {
		Instruction_t *insn=*it;
		virtual_offset_t addr=insn->GetAddress()->GetVirtualOffset();

		/* lookup in the list of targets */
		if(targets.find(addr)!=targets.end())
		{
			AddressID_t* newaddr = new AddressID_t;
			newaddr->SetFileID(insn->GetAddress()->GetFileID());
			newaddr->SetVirtualOffset(insn->GetAddress()->GetVirtualOffset());
			
			insn->SetIndirectBranchTargetAddress(newaddr);
			firp->GetAddresses().insert(newaddr);
		}
	}
}

void get_instruction_targets(FileIR_t *firp, EXEIO::exeio* elfiop, const set<virtual_offset_t>& thunk_bases)
{

        for(
                set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
                it!=firp->GetInstructions().end();
                ++it
           )
        {
                Instruction_t *insn=*it;
                DISASM disasm;
                virtual_offset_t instr_len = insn->Disassemble(disasm);

                assert(instr_len==insn->GetDataBits().size());

		check_for_PIC_switch_table32_type2(insn,disasm, elfiop, thunk_bases);
		check_for_PIC_switch_table32_type3(insn,disasm, elfiop, thunk_bases);
//		check_for_PIC_switch_table64(firp,insn,disasm, elfiop);
		if (firp->GetArchitectureBitWidth()==32)
			check_for_PIC_switch_table32(firp, insn,disasm, elfiop, thunk_bases);
		else if (firp->GetArchitectureBitWidth()==64)
			check_for_PIC_switch_table64(firp, insn,disasm, elfiop);
		else
			assert(0);

		if (jmptables.count(insn) == 0)
			check_for_nonPIC_switch_table(firp, insn,disasm, elfiop);

		if (jmptables.count(insn) == 0)
			check_for_nonPIC_switch_table_pattern2(firp, insn,disasm, elfiop);

		// assign hellnode type to indirect jmps that are not detected
		// to be switch tables
		if (jmptables.count(insn) == 0)
			check_for_indirect_jmp(firp, insn);

		// assign special hellnode type to indirect calls
		check_for_indirect_call(firp, insn);
		check_for_ret(firp, insn);

		/* other branches can't indicate an indirect branch target */
		if(disasm.Instruction.BranchType)
			continue;

		/* otherwise, any immediate is a possible branch target */
		possible_target(disasm.Instruction.Immediat);

		handle_argument(&disasm.Argument1, insn);
		handle_argument(&disasm.Argument2, insn);
		handle_argument(&disasm.Argument3, insn);
	}
}

void get_executable_bounds(FileIR_t *firp, const section* shdr)
{

	/* not a loaded section */
	if( !shdr->isLoadable()) 
		return;

	/* loaded, and contains instruction, record the bounds */
	if( !shdr->isExecutable() )
		return;

	virtual_offset_t first=shdr->get_address();
	virtual_offset_t second=shdr->get_address()+shdr->get_size();

	bounds.insert(pair<virtual_offset_t,virtual_offset_t>(first,second));


}

void infer_targets(FileIR_t *firp, section* shdr)
{
//	int flags = shdr->get_flags();

	if( ! shdr->isLoadable()) // (flags & SHF_ALLOC) != SHF_ALLOC)
		/* not a loaded section */
		return;

	if( shdr->isExecutable() ) //(flags & SHF_EXECINSTR) == SHF_EXECINSTR)
		/* loaded, but contains instruction.  we'll look through the VariantIR for this section. */
		return;

	/* if the type is NOBITS, then there's no actual data to look through */
	if(shdr->isBSS() ) // get_type()==SHT_NOBITS)
		return;


	cout<<"Checking section "<<shdr->get_name() <<endl;

	const char* data=shdr->get_data() ; // C(char*)malloc(shdr->sh_size);

	assert(arch_ptr_bytes()==4 || arch_ptr_bytes()==8);
	for(int i=0;i+arch_ptr_bytes()<=shdr->get_size();i++)
	{
		// even on 64-bit, pointers might be stored as 32-bit, as a 
		// elf object has the 32-bit limitations.
		// there's no real reason to look for 64-bit pointers 
		uintptr_t p=0;
		if(arch_ptr_bytes()==4)
			p=*(int*)&data[i];
		else
			p=*(virtual_offset_t*)&data[i];	// 64 or 32-bit depending on sizeof uintptr_t, may need porting for cross platform analysis.
		possible_target(p, i+shdr->get_address());
	}

}


void print_targets()
{
	int j=0;
	for(
		set<virtual_offset_t>::iterator it=targets.begin();
		it!=targets.end();
		++it, j++
	   )
	{
		virtual_offset_t target=*it;
	
		cout<<std::hex<<target;
		if(j%10 == 0)
			cout<<endl; 
		else
			cout<<", ";
	}

	cout<<endl;
}

/* 
 *
 * add_num_handle_fn_watches - 
 *
 *  This function is a quick and dirty way to ensure that 
 *  certain function call watches are not interfered with by ILR
 *  This is done by marking the functions of interest as indirect targets
 *  so that they receive a spri rule of the form <original_addr> -> <newaddr>
 * 
 *  Current function list:  
 *      fread, fread_unlocked, 
 *      fwrite, fwrite_unlocked, 
 *      strncpy, strncat, strncmp, strxfrm
 *      memcpy, memmove, memcmp, memchr, memrchr, memset
 *      wcsncpy, wcsncat, wcsncmp, wcsxfrm
 *      wmemcpy, wmemmove, wmemcmp, wmemchr, memset
 *
 */
void add_num_handle_fn_watches(FileIR_t * firp)
{
    	/* Loop over the set of functions */
    for(
       	set<Function_t*>::const_iterator it=firp->GetFunctions().begin();
       	it!=firp->GetFunctions().end();
        ++it
        )
    {
        Function_t *func=*it;
        char *funcname=(char *)func->GetName().c_str();
	if(!func->GetEntryPoint())
		continue;
        virtual_offset_t the_offset=func->GetEntryPoint()->GetAddress()->GetVirtualOffset();

        /* macro to facilitate the checking */
#define CHECK_FN(fname)                         \
        if(strcmp(#fname, funcname)==0)         \
        {                                       \
            possible_target(the_offset);        \
        }

        /* 
         * if one that we want to watch, 
         * mark it as a possible target 
         */
        CHECK_FN(fread);
        CHECK_FN(_IO_fread);
        CHECK_FN(fread_unlocked);
        CHECK_FN(fwrite);
        CHECK_FN(_IO_fwrite);
        CHECK_FN(fwrite_unlocked);
        CHECK_FN(strncpy);
        CHECK_FN(strncmp);
        CHECK_FN(strxfrm);
        CHECK_FN(memcpy);
        CHECK_FN(memmove);
        CHECK_FN(memcmp);
        CHECK_FN(memchr);
        CHECK_FN(memrchr);
        CHECK_FN(memset);
        CHECK_FN(wcsncpy);
        CHECK_FN(wcsxfrm);
        CHECK_FN(wmemcpy);
        CHECK_FN(wmemmove);
        CHECK_FN(wmemcmp);
        CHECK_FN(wmemchr);
        CHECK_FN(wmemset);
        
    }

}

set<Instruction_t*> find_in_function(string needle, Function_t *haystack)
{
	DISASM disasm;
	regex_t preg;
	set<Instruction_t*>::const_iterator fit;
	set<Instruction_t*> found_instructions;

	assert(0 == regcomp(&preg, needle.c_str(), REG_EXTENDED));

	fit = haystack->GetInstructions().begin();
	for (fit; fit != haystack->GetInstructions().end(); fit++)
	{
		Instruction_t *candidate = *fit;
		candidate->Disassemble(disasm);

		// check it's the requested type
		if(regexec(&preg, disasm.CompleteInstr, 0, NULL, 0) == 0)
		{
			found_instructions.insert(candidate);
		}
	}
	regfree(&preg);
	return found_instructions;
}

bool backup_until(const char* insn_type_regex, Instruction_t *& prev, Instruction_t* orig, bool recursive=false)
{
	DISASM disasm;
	prev=orig;
	regex_t preg;

	assert(0 == regcomp(&preg, insn_type_regex, REG_EXTENDED));

	while(preds[prev].size()==1)
	{
		// get the only item in the list.
		prev=*(preds[prev].begin());

       		// get I7's disassembly
       		prev->Disassemble(disasm);

       		// check it's the requested type
       		if(regexec(&preg, disasm.CompleteInstr, 0, NULL, 0) == 0)
		{
			regfree(&preg);
			return true;
		}

		// otherwise, try backing up again.
	}
	if(recursive)
	{
		Instruction_t* myprev=prev;
		// can't just use prev because recursive call will update it.
		for(InstructionSet_t::iterator it=preds[myprev].begin();
			it!=preds[myprev].end(); ++it)
		{
			Instruction_t* pred=*it;
       		
			pred->Disassemble(disasm);
       			// check it's the requested type
       			if(regexec(&preg, disasm.CompleteInstr, 0, NULL, 0) == 0)
			{
				regfree(&preg);
				return true;
			}
			if(backup_until(insn_type_regex, prev, pred))
			{
				regfree(&preg);
				return true;
			}
			// reset for next call
			prev=myprev;
		}
	}
	regfree(&preg);
	return false;
}


/*
 * check_for_PIC_switch_table32 - look for switch tables in PIC code for 32-bit code.
 */
void check_for_PIC_switch_table32(FileIR_t *firp, Instruction_t* insn, DISASM disasm, EXEIO::exeio* elfiop, const set<virtual_offset_t> &thunk_bases)
{
#if 0

/* here's typical code */

I1: 080a9037 <gedit_floating_slider_get_type+0x607> call   0806938e <_gedit_app_ready+0x8e>  	// ebx=080a903c
I2: 080a903c <gedit_floating_slider_get_type+0x60c> add    $0x45fb8,%ebx			// ebx=<module_start>
...
I3: 080a90f8 <gedit_floating_slider_get_type+0x6c8> mov    -0x1ac14(%ebx,%esi,4),%eax		// table_start=<module_start-0x1ac14
												// table_offset=eax=table_start[esi]
I4: 080a90ff <gedit_floating_slider_get_type+0x6cf> add    %ebx,%eax				// switch_case=eax=<module_start>+table_offset
I5: 080a9101 <gedit_floating_slider_get_type+0x6d1> jmp    *%eax				// jump switch_case
...
I6: 0806938e <_gedit_app_ready+0x8e> mov    (%esp),%ebx
I7: 08069391 <_gedit_app_ready+0x91> ret


/* However, since the thunk and the switch table can be (and sometimes are) very control-flow distinct, 
 * we just collect all the places where a module can start by examining all the thunk/add pairs.
 * After that, we look for all jumps that match the I3-I5 pattern, and consider the offset against each
 * module start.  If we find that there are possible targets at <module_start>+table_offset, we record them.
 */

#endif

        Instruction_t* I5=insn;
        Instruction_t* Icmp=NULL;
        Instruction_t* I4=NULL;
        Instruction_t* I3=NULL;
        // check if I5 is a jump
        if(strstr(disasm.Instruction.Mnemonic, "jmp")==NULL)
		return;

	// return if it's a jump to a constant address, these are common
        if(disasm.Argument1.ArgType&CONSTANT_TYPE)
		return;

	// return if it's a jump to a memory address
        if(disasm.Argument1.ArgType&MEMORY_TYPE)
		return;

	// has to be a jump to a register now

	// backup and find the instruction that's an add before I8 
	if(!backup_until("add", I4, I5))
		return;

	// backup and find the instruction that's an movsxd before I7
	if(!backup_until("mov", I3, I4))
		return;

	int table_size = 0;
	if (!backup_until("cmp", Icmp, I3))
	{
		cerr<<"pic32: could not find size of switch table"<<endl;
		table_size=std::numeric_limits<int>::max();
		// set table_size to be very large, so we can still do pinning appropriately
	}
	else
	{
		DISASM dcmp;
		Icmp->Disassemble(dcmp);
		table_size = dcmp.Instruction.Immediat;
		if(table_size<=0)
			table_size=std::numeric_limits<int>::max();
	}

	// grab the offset out of the lea.
	DISASM d2;
	I3->Disassemble(d2);

	// get the offset from the thunk
	virtual_offset_t table_offset=d2.Argument2.Memory.Displacement;
	if(table_offset==0)
		return;

cout<<hex<<"Found switch dispatch at "<<I3->GetAddress()->GetVirtualOffset()<< " with table_offset="<<table_offset<<" and table_size="<<table_size<<endl;
		
	/* iterate over all thunk_bases/module_starts */
	for(set<virtual_offset_t>::iterator it=thunk_bases.begin(); it!=thunk_bases.end(); ++it)
	{
		virtual_offset_t thunk_base=*it;
		virtual_offset_t table_base=*it+table_offset;

		// find the section with the data table
        	EXEIO::section *pSec=find_section(table_base,elfiop);
		if(!pSec)
			continue;

		// if the section has no data, abort 
        	const char* secdata=pSec->get_data();
		if(!secdata)
			continue;

		// get the base offset into the section
        	virtual_offset_t offset=table_base-pSec->get_address();
		int i;
		for(i=0;i<3;i++)
		{
                	if(offset+i*4+sizeof(int) > pSec->get_size())
                        	break;

                	const int *table_entry_ptr=(const int*)&(secdata[offset+i*4]);
                	int table_entry=*table_entry_ptr;

			if(!is_possible_target(thunk_base+table_entry,table_base+i*4))
				break;	
		}
		/* did we finish the loop or break out? */
		if(i==3)
		{
			set<Instruction_t *> ibtargets;

			if(getenv("IB_VERBOSE")!=0)
				cout<<"Found switch table (thunk-relative) at "<<hex<<table_base+table_offset<<endl;
			// finished the loop.
			for(i=0;true;i++)
			{
                		if(offset+i*4+sizeof(int) > pSec->get_size())
                        		break;
	
                		const int32_t *table_entry_ptr=(const int32_t*)&(secdata[offset+i*4]);
                		virtual_offset_t table_entry=*table_entry_ptr;
	
				if(getenv("IB_VERBOSE")!=0)
					cout<<"Found switch table (thunk-relative) entry["<<dec<<i<<"], "<<hex<<thunk_base+table_entry<<endl;

				if(!possible_target(thunk_base+table_entry,table_base+i*4))
					break;

				Instruction_t *ibtarget = lookupInstruction(firp, thunk_base+table_entry);
				if (ibtarget && ibtargets.size() <= table_size)
				{
					ibtargets.insert(ibtarget);
				}
			}

			// valid switch table? may or may not have default: in the switch
			// table size = 8, #entries: 9 b/c of default
			cout << "pic32 (base pattern): table size: " << table_size << " ibtargets.size: " << ibtargets.size() << endl;
			if (table_size == ibtargets.size() || table_size == (ibtargets.size()-1))
			{
				cout << "pic32 (base pattern): valid switch table detected" << endl;
				jmptables[I5] = ibtargets;
			}
		}
		else
		{
			if(getenv("IB_VERBOSE")!=0)
				cout<<"Found that  "<<hex<<table_base+table_offset<<endl;
		}

		// now, try next thunk base 
	}


}

void check_for_PIC_switch_table32_type2(Instruction_t* insn, DISASM disasm, EXEIO::exeio* elfiop, const set<virtual_offset_t> &thunk_bases)
{
#if 0

/* here's typical code */
I1:   0x8098ffc <text_handler+33>: cmp    eax,0x8
I2:   0x8098fff <text_handler+36>: ja     0x8099067 <text_handler+140>
I3:   0x8099001 <text_handler+38>: lea    ecx,[ebx-0x21620]
I4:   0x8099007 <text_handler+44>: add    ecx,DWORD PTR [ebx+eax*4-0x21620]
I5:   0x809900e <text_handler+51>: jmp    ecx
#endif

        Instruction_t* I5=insn;
        Instruction_t* I4=NULL;
        Instruction_t* I3=NULL;
        // check if I5 is a jump
        if(strstr(disasm.Instruction.Mnemonic, "jmp")==NULL)
		return;

	// return if it's a jump to a constant address, these are common
        if(disasm.Argument1.ArgType&CONSTANT_TYPE)
		return;

	// return if it's a jump to a memory address
        if(disasm.Argument1.ArgType&MEMORY_TYPE)
		return;

	// has to be a jump to a register now

	// backup and find the instruction that's an add before I8 
	if(!backup_until("add", I4, I5))
		return;

	// backup and find the instruction that's an movsxd before I7
	if(!backup_until("lea", I3, I4))
		return;

	// grab the offset out of the lea.
	DISASM d2;
	I3->Disassemble(d2);

	// get the offset from the thunk
	virtual_offset_t table_offset=d2.Argument2.Memory.Displacement;
	if(table_offset==0)
		return;

cout<<hex<<"Found (type2) switch dispatch at "<<I3->GetAddress()->GetVirtualOffset()<< " with table_offset="<<table_offset<<endl;
		
	/* iterate over all thunk_bases/module_starts */
	for(set<virtual_offset_t>::iterator it=thunk_bases.begin(); it!=thunk_bases.end(); ++it)
	{
		virtual_offset_t thunk_base=*it;
		virtual_offset_t table_base=*it+table_offset;

		// find the section with the data table
        	EXEIO::section *pSec=find_section(table_base,elfiop);
		if(!pSec)
			continue;

		// if the section has no data, abort 
        	const char* secdata=pSec->get_data();
		if(!secdata)
			continue;

		// get the base offset into the section
        	virtual_offset_t offset=table_base-pSec->get_address();
		int i;
		for(i=0;i<3;i++)
		{
                	if(offset+i*4+sizeof(int) > pSec->get_size())
                        	break;

                	const int32_t *table_entry_ptr=(const int32_t*)&(secdata[offset+i*4]);
                	virtual_offset_t table_entry=*table_entry_ptr;

cout<<"Checking target base:" << std::hex << table_base+table_entry << ", " << table_base+i*4<<endl;
			if(!is_possible_target(table_base+table_entry,table_base+i*4))
				break;	
		}
		/* did we finish the loop or break out? */
		if(i==3)
		{
			if(getenv("IB_VERBOSE")!=0)
				cout<<"Found switch table (pic3, type2) (thunk-relative) at "<<hex<<table_base+table_offset<<endl;
			// finished the loop.
			for(i=0;true;i++)
			{
                		if(offset+i*4+sizeof(int) > pSec->get_size())
                        		break;
	
                		const int32_t *table_entry_ptr=(const int32_t*)&(secdata[offset+i*4]);
                		virtual_offset_t table_entry=*table_entry_ptr;
	
				if(getenv("IB_VERBOSE")!=0)
					cout<<"Found switch table (thunk-relative) entry["<<dec<<i<<"], "<<hex<<table_base+table_entry<<endl;
				if(!possible_target(table_base+table_entry,table_base+i*4))
					break;
			}
		}
		else
		{
			if(getenv("IB_VERBOSE")!=0)
				cout<<"Found that  "<<hex<<table_base+table_offset<<endl;
		}

		// now, try next thunk base 
	}


}

void check_for_PIC_switch_table32_type3(Instruction_t* insn, DISASM disasm, EXEIO::exeio* elfiop, const set<virtual_offset_t> &thunk_bases)
{
#if 0

/* here's typical code */
	   0x809900e <text_handler+51>: jmp    [eax*4 + 0x8088208]
#endif

        Instruction_t* I5=insn;
        // check if I5 is a jump
        if(strstr(disasm.Instruction.Mnemonic, "jmp")==NULL)
		return;

	// return if it's not a jump to a memory address
        if(!(disasm.Argument1.ArgType&MEMORY_TYPE))
		return;

	/* return if there's no displacement */
        if(disasm.Argument1.Memory.Displacement==0)
		return;

	// grab the table base out of the jmp.
	virtual_offset_t table_base=disasm.Argument1.Memory.Displacement;
	if(table_base==0)
		return;

cout<<hex<<"Found (type3) switch dispatch at "<<I5->GetAddress()->GetVirtualOffset()<< " with table_base="<<table_base<<endl;
		
	{
		// find the section with the data table
        	EXEIO::section *pSec=find_section(table_base,elfiop);
		if(!pSec)
			return;

		// if the section has no data, abort 
        	const char* secdata=pSec->get_data();
		if(!secdata)
			return;

		// get the base offset into the section
        	virtual_offset_t offset=table_base-pSec->get_address();
		int i;
		for(i=0;i<3;i++)
		{
                	if(offset+i*4+sizeof(int) > pSec->get_size())
                        	return;

                	const int *table_entry_ptr=(const int*)&(secdata[offset+i*4]);
                	virtual_offset_t table_entry=*table_entry_ptr;

cout<<"Checking target base:" << std::hex << table_entry << ", " << table_base+i*4<<endl;
			if(!is_possible_target(table_entry,table_base+i*4))
				return;	
		}
		/* did we finish the loop or break out? */
		if(i==3)
		{
			if(getenv("IB_VERBOSE")!=0)
				cout<<"Found switch table (pic3, type3) (thunk-relative) at "<<hex<<table_base<<endl;
			// finished the loop.
			for(i=0;true;i++)
			{
                		if(offset+i*4+sizeof(int) > pSec->get_size())
                        		return;
	
                		const int *table_entry_ptr=(const int*)&(secdata[offset+i*4]);
                		virtual_offset_t table_entry=*table_entry_ptr;
	
				if(getenv("IB_VERBOSE")!=0)
					cout<<"Found switch table (thunk-relative) entry["<<dec<<i<<"], "<<hex<<table_entry<<endl;
				if(!possible_target(table_entry,table_base+i*4))
					return;
			}
		}
		else
		{
			if(getenv("IB_VERBOSE")!=0)
				cout<<"Found that  "<<hex<<table_base<<endl;
		}

		// now, try next thunk base 
	}


}



/* check if this instruction is an indirect jump via a register,
 * if so, see if we can trace back a few instructions to find a
 * the start of the table.
 */
void check_for_PIC_switch_table64(FileIR_t* firp, Instruction_t* insn, DISASM disasm, EXEIO::exeio* elfiop)
{
/* here's the pattern we're looking for */
#if 0
I1:   0x000000000044425a <+218>:        cmp    DWORD PTR [rax+0x8],0xd   // bounds checking code, 0xd cases. switch(i) has i stored in [rax+8] in this e.g.
I2:   0x000000000044425e <+222>:        jbe    0x444320 <_gedit_tab_get_icon+416>
<new bb>
I3:   0x0000000000444264 <+228>:        mov    rdi,rbp // default case, also jumped to via indirect branch below
<snip (doesn't fall through)>
I4:   0x0000000000444320 <+416>:        mov    edx,DWORD PTR [rax+0x8]		# load from memory into index reg EDX.

THIS ONE
I5:   0x0000000000444323 <+419>:        lea    rax,[rip+0x3e1b6]        # 0x4824e0
I6:   0x000000000044432a <+426>:        movsxd rdx,DWORD PTR [rax+rdx*4]
I7:   0x000000000044432e <+430>:        add    rax,rdx  // OR: lea rax, [rdx+rax]
I8:   0x0000000000444331 <+433>:        jmp    rax      // relatively standard switch dispatch code


D1:   0x4824e0: .long 0x4824e0-L1       // L1-LN are labels in the code where case statements start.
D2:   0x4824e4: .long 0x4824e0-L2
..
DN:   0x4824XX: .long 0x4824e0-LN
#endif


	// for now, only trying to find I4-I8.  ideally finding I1 would let us know the size of the
	// jump table.  We'll figure out N by trying targets until they fail to produce something valid.

	Instruction_t* I8=insn;
	Instruction_t* I7=NULL;
	Instruction_t* I6=NULL;
	Instruction_t* I5=NULL;
	Instruction_t* I1=NULL;
	// check if I8 is a jump
	if(strstr(disasm.Instruction.Mnemonic, "jmp")==NULL)
		return;

	// return if it's a jump to a constant address, these are common
	if(disasm.Argument1.ArgType&CONSTANT_TYPE)
		return;
	// return if it's a jump to a memory address
	if(disasm.Argument1.ArgType&MEMORY_TYPE)
		return;

	// has to be a jump to a register now

	/* 
	 * This is the instruction that adds the table value
	 * to the base address of the table. The result is 
	 * the target address of the jump.
	 *
	 * Backup and find the instruction that's an add or lea before I8.
	 * TODO: Should we check to make sure that the registers match?
	 */
	if(!backup_until("(add|lea)", I7, I8))
		return;

	I7->Disassemble(disasm);

	// Check if lea instruction is being used as add (scale=1, disp=0)
	if(strstr(disasm.Instruction.Mnemonic, "lea"))
	{
		if(!(disasm.Argument2.ArgType&MEMORY_TYPE))
		if(!(disasm.Argument2.Memory.Scale == 1 && disasm.Argument2.Memory.Displacement == 0))
		return;
	} 
	// backup and find the instruction that's an movsxd before I7
	/*
	 * This instruction will contain the register names for
	 * the index and the address of the base of the table
	 */
	if(!backup_until("movsxd", I6, I7))
		return;
	
	I6->Disassemble(disasm);
	if( (disasm.Argument2.ArgType&MEMORY_TYPE)	 != MEMORY_TYPE)
		return;


	// 64-bit register names are OK here, because this pattern already won't apply to 32-bit code.
	/*
	 * base_reg is the register that holds the address
	 * for the base of the jump table.
	 */
	string base_reg="";
	switch(disasm.Argument2.Memory.BaseRegister)
	{
		case REG0: base_reg="rax"; break;
		case REG1: base_reg="rcx"; break;
		case REG2: base_reg="rdx"; break;
		case REG3: base_reg="rbx"; break;
		case REG4: base_reg="rsp"; break;
		case REG5: base_reg="rbp"; break;
		case REG6: base_reg="rsi"; break;
		case REG7: base_reg="rdi"; break;
		case REG8: base_reg="r8"; break;
		case REG9: base_reg="r9"; break;
		case REG10: base_reg="r10"; break;
		case REG11: base_reg="r11"; break;
		case REG12: base_reg="r12"; break;
		case REG13: base_reg="r13"; break;
		case REG14: base_reg="r14"; break;
		case REG15: base_reg="r15"; break;
		default: assert(0);
	}
	string lea_string="lea ";
	lea_string+=base_reg;
	

	/* 
	 * This is the instruction that loads the address of the base
	 * of the table into a register.
	 *
	 * backup and find the instruction that's an lea before I6;
	 * make sure we match the register,
	 * and allow recursion in the search (last parameter as true).
	 *
	 */

	 /*
	  * Convert to return set
		*/

	set<Instruction_t*>::const_iterator found_leas_it;
	set<Instruction_t*> found_leas;
	
	
	if (I6->GetFunction())
	{
		cout << "Using find_in_function method." << endl;
		found_leas=find_in_function(lea_string,I6->GetFunction());
	}
	else
	{
		cout << "Using fallback method." << endl;
		if (backup_until(lea_string.c_str(), I5, I6, true))
		{
			found_leas.insert(I5);
		}
	}
	if (found_leas.empty())
	{
		/*
		 * TODO: Write this to warning.txt
		 */
		cout << "WARNING: No I5 candidates found!" << endl;
	}

	/*
	 * Check each one that is returned.
	 */
	found_leas_it = found_leas.begin();
	for (found_leas_it; found_leas_it != found_leas.end(); found_leas_it++) 
	{
		Instruction_t *I5_cur = *found_leas_it;
		I5_cur->Disassemble(disasm);

		if(!(disasm.Argument2.ArgType&MEMORY_TYPE))
			//return;
			continue;
		if(!(disasm.Argument2.ArgType&RELATIVE_))
			//return;
			continue;

		// note that we'd normally have to add the displacement to the
		// instruction address (and include the instruction's size, etc.
		// but, fix_calls has already removed this oddity so we can relocate
		// the instruction.
		virtual_offset_t D1=strtol(disasm.Argument2.ArgMnemonic, NULL, 16);

		// find the section with the data table
		EXEIO::section *pSec=find_section(D1,elfiop);

		// sanity check there's a section
		if(!pSec)
			//return;
			continue;

		const char* secdata=pSec->get_data();

		// if the section has no data, abort 
		if(!secdata)
			//return;
			continue;

		int table_size = 0;
		if(!backup_until("cmp", I1, I5_cur))
		{
			cout<<"pic64: could not find size of switch table"<<endl;

			// we set the table_size variable to max_int so that we can still do pinning, 
			// but we won't do the switch identification.
			table_size=std::numeric_limits<int>::max();
		}
		else
		{
			DISASM d1;
			I1->Disassemble(d1);
			table_size = d1.Instruction.Immediat;
			if (table_size <= 0)
				// set table_size to be very large, so we can still do pinning appropriately
				table_size=std::numeric_limits<int>::max();
		}

		set<Instruction_t *> ibtargets;
		virtual_offset_t offset=D1-pSec->get_address();
		int entry=0;
		do
		{
			// check that we can still grab a word from this section
			if(offset+sizeof(int) > pSec->get_size())
				break;

			const int *table_entry_ptr=(const int*)&(secdata[offset]);
			virtual_offset_t table_entry=*table_entry_ptr;

			if(!possible_target(D1+table_entry))
				break;

			if(getenv("IB_VERBOSE"))
			{
				cout<<"Found possible table entry, at: "<< std::hex << I8->GetAddress()->GetVirtualOffset()
			    		<< " insn: " << disasm.CompleteInstr<< " d1: "
			    		<< D1 << " table_entry:" << table_entry 
			    		<< " target: "<< D1+table_entry << std::dec << endl;
			}

			Instruction_t *ibtarget = lookupInstruction(firp, D1+table_entry);
			if (ibtarget && ibtargets.size() <= table_size)
			{
				if(getenv("IB_VERBOSE"))
					cout << "jmp table [" << entry << "]: " << hex << table_entry << dec << endl;
				ibtargets.insert(ibtarget);
			}
			else
			{
				if(getenv("IB_VERBOSE"))
					cout << "      INVALID target" << endl;
				break;
			}
			offset+=sizeof(int);
			entry++;
		} while ( entry<=table_size);

		
		// valid switch table? may or may not have default: in the switch
		// table size = 8, #entries: 9 b/c of default
		cout << "pic64: detected table size (max_int means no found): 0x"<< hex << table_size << " #entries: 0x" << entry << " ibtargets.size: " << ibtargets.size() << endl;

		// note that there may be an off-by-one error here as table size depends on whether instruction I2 is a jb or jbe.
		if (table_size == ibtargets.size() || table_size == (ibtargets.size()-1))
		{
			cout << "pic64: valid switch table detected" << endl;
			jmptables[I8] = ibtargets;
		}
	}
}

/*
  switch table pattern (non-PIC):
      40062a:	8d 40 ec             	lea  eax,[rax-0x14]
  I1: 40062d:	83 f8 08             	cmp  eax,0x8                     'size
  I2: 400630:	0f 87 98 00 00 00    	ja   4006ce <main+0xbb>
      400636:	89 c0                	mov  eax,eax
  I3: 400638:	ff 24 c5 10 08 40 00	jmp  QWORD PTR [rax*8+0x400810]  'branch
      40063f:	bf a9 07 40 00       	mov  edi,0x4007a9
      400644:	e8 67 fe ff ff       	call 4004b0 <puts@plt>
      400649:	e9 96 00 00 00       	jmp  4006e4 <main+0xd1>

	nb: handles both 32 and 64 bit
*/
void check_for_nonPIC_switch_table_pattern2(FileIR_t* firp, Instruction_t* insn, DISASM disasm, EXEIO::exeio* elfiop)
{
	Instruction_t *I1 = NULL;
	Instruction_t *IJ = insn;

	assert(IJ);

	// check if IJ is a jump
	if(strstr(disasm.Instruction.Mnemonic, "jmp")==NULL)
		return;

	// look for a memory type
	if(!(disasm.Argument1.ArgType&MEMORY_TYPE))
		return;

	// make sure there's a scaling factor
	if (disasm.Argument1.Memory.Scale < 4)
		return;

	// extract start of jmp table
	virtual_offset_t table_offset = disasm.Argument1.Memory.Displacement;
	if(table_offset==0)
		return;

	cout<<hex<<"(nonPIC-pattern2): Found switch dispatch at 0x"<<hex<<IJ->GetAddress()->GetVirtualOffset()<< " with table_offset="<<hex<<table_offset<<dec<<endl;

	if(!backup_until("cmp", I1, IJ))
	{
		cout<<"(nonPIC): could not find size of switch table"<<endl;
		return;
	}

	// extract size off the comparison
	// make sure not off by one
	DISASM d1;
	I1->Disassemble(d1);
	virtual_offset_t table_size = d1.Instruction.Immediat;

	if (table_size <= 0) return;

	cout<<"(nonPIC-pattern2): size of jmp table: "<< table_size << endl;

	// find the section with the data table
    EXEIO::section *pSec=find_section(table_offset,elfiop);
	if(!pSec)
	{
		return;
	}

	// if the section has no data, abort 
	const char* secdata=pSec->get_data();
	if(!secdata)
		return;

	// get the base offset into the section
    	virtual_offset_t offset=table_offset-pSec->get_address();
	int i;

	set<Instruction_t*> ibtargets;
	for(i=0;i<table_size;++i)
	{
		if(offset+i*arch_ptr_bytes()+sizeof(int) > pSec->get_size())
		{
			cout << "jmp table outside of section range ==> invalid switch table" << endl;
			return;
		}

		const virtual_offset_t *table_entry_ptr=(const virtual_offset_t*)&(secdata[offset+i*arch_ptr_bytes()]);
		virtual_offset_t table_entry=*table_entry_ptr;

		Instruction_t *ibtarget = lookupInstruction(firp, table_entry);
		if (!ibtarget) {
			if(getenv("IB_VERBOSE"))
				cout << "0x" << hex << table_entry << " is not an instruction, invalid switch table" << endl;
			return;
		}

		if(getenv("IB_VERBOSE"))
			cout << "jmp table [" << i << "]: " << hex << table_entry << dec << endl;
		ibtargets.insert(ibtarget);
	}

	cout << "(non-PIC) valid switch table found" << endl;

	jmptables[IJ] = ibtargets;
}


/*
  Handles the following switch table pattern:
  I1: 400518:   83 7d ec 0c             cmpl   $0xc,-0x14(%rbp)          'size
  I2: 40051c:   0f 87 b7 00 00 00       ja     4005d9 <main+0xe5>        
  I3: 400522:   8b 45 ec                mov    -0x14(%rbp),%eax 
  I4: 400525:   48 8b 04 c5 20 07 40    mov    0x400720(,%rax,8),%rax    'start jump table
      40052c:   00  
  IJ: 40052d:   ff e0                   jmpq   *%rax                     'indirect branch
      40052f:   c7 45 f4 00 00 00 00    movl   $0x0,-0xc(%rbp)
      400536:   c7 45 f8 03 00 00 00    movl   $0x3,-0x8(%rbp)

	nb: handles both 32 and 64 bit
*/
void check_for_nonPIC_switch_table(FileIR_t* firp, Instruction_t* insn, DISASM disasm, EXEIO::exeio* elfiop)
{
	Instruction_t *I1 = NULL;
	Instruction_t *I2 = NULL;
	Instruction_t *I4 = NULL;
	Instruction_t *IJ = insn;

	if (!IJ) return;

	// check if IJ is a jump
	if(strstr(disasm.Instruction.Mnemonic, "jmp")==NULL)
		return;

	// return if it's a jump to a constant address, these are common
	if(disasm.Argument1.ArgType&CONSTANT_TYPE)
		return;

	// return if it's a jump to a memory address
	if(disasm.Argument1.ArgType&MEMORY_TYPE)
		return;

	// has to be a jump to a register now

	// backup and find the instruction that's a mov
	if(!backup_until("mov", I4, IJ))
		return;

	// extract start of jmp table
	DISASM d4;
	I4->Disassemble(d4);

	// make sure there's a scaling factor
	if (d4.Argument2.Memory.Scale < 4)
		return;

	virtual_offset_t table_offset=d4.Argument2.Memory.Displacement;
	if(table_offset==0)
		return;

	if(getenv("IB_VERBOSE"))
		cout<<hex<<"(nonPIC): Found switch dispatch at 0x"<<hex<<I4->GetAddress()->GetVirtualOffset()<< " with table_offset="<<hex<<table_offset<<dec<<endl;

	if(!backup_until("cmp", I1, I4))
	{
		cout<<"(nonPIC): could not find size of switch table"<<endl;
		return;
	}

	// extract size off the comparison
	// make sure not off by one
	DISASM d1;
	I1->Disassemble(d1);
	virtual_offset_t table_size = d1.Instruction.Immediat;
	if (table_size <= 0) return;

	if(getenv("IB_VERBOSE"))
		cout<<"(nonPIC): size of jmp table: "<< table_size << endl;

	// find the section with the data table
	EXEIO::section *pSec=find_section(table_offset,elfiop);
	if(!pSec)
	{
		cout<<hex<<"(nonPIC): could not find jump table in section"<<endl;
		return;
	}

	// if the section has no data, abort 
	const char* secdata=pSec->get_data();
	if(!secdata)
		return;

	// get the base offset into the section
	virtual_offset_t offset=table_offset-pSec->get_address();
	int i;

	if(getenv("IB_VERBOSE"))
		cout << hex << "offset: " << offset << " arch bit width: " << dec << firp->GetArchitectureBitWidth() << endl;

	set<Instruction_t*> ibtargets;
	for(i=0;i<table_size;++i)
	{
		if(offset+i*arch_ptr_bytes()+sizeof(int) > pSec->get_size())
		{
			cout << "jmp table outside of section range ==> invalid switch table" << endl;
			return;
		}

		virtual_offset_t table_entry=0;
		if (firp->GetArchitectureBitWidth()==32)
		{
			const int *table_entry_ptr=(const int*)&(secdata[offset+i*arch_ptr_bytes()]);
			table_entry=*table_entry_ptr;
		}
		else if (firp->GetArchitectureBitWidth()==64)
		{
			const virtual_offset_t *table_entry_ptr=(const virtual_offset_t*)&(secdata[offset+i*arch_ptr_bytes()]);
			table_entry=*table_entry_ptr;
		}
		else 
			assert(0 && "Unknown arch size.");

		Instruction_t *ibtarget = lookupInstruction(firp, table_entry);
		if (!ibtarget) {
			if(getenv("IB_VERBOSE"))
				cout << "0x" << hex << table_entry << " is not an instruction, invalid switch table" << endl;
			return;
		}

		if(getenv("IB_VERBOSE"))
			cout << "jmp table [" << i << "]: " << hex << table_entry << dec << endl;
		ibtargets.insert(ibtarget);
	}

	cout << "(non-PIC) valid switch table found" << endl;
	jmptables[IJ] = ibtargets;
}

void icfs_init(FileIR_t* firp)
{
	assert(firp);
	hellnode_tgts = new ICFS_t(HELLNODE_ID, false);
	indirect_calls = new ICFS_t(INDIRECT_CALLS_ID, false); 
	firp->GetAllICFS().insert(hellnode_tgts);
	firp->GetAllICFS().insert(indirect_calls);
}

void icfs_set_indirect_calls(FileIR_t* const firp, ICFS_t* const targets)
{
	assert(firp && targets);
    for(
       	FunctionSet_t::const_iterator it=firp->GetFunctions().begin();
       	it!=firp->GetFunctions().end();
        ++it
        )
    {
        Function_t *func=*it;
		if(!func->GetEntryPoint())
			continue;
		targets->insert(func->GetEntryPoint());
	}
}

void icfs_set_hellnode_targets(FileIR_t* const firp, ICFS_t* const targets)
{
	assert(firp && targets);
	for(
		InstructionSet_t::const_iterator it=firp->GetInstructions().begin();
			it!=firp->GetInstructions().end(); ++it)
	{
		Instruction_t* insn=*it;
		if(insn->GetIndirectBranchTargetAddress())
		{
			targets->insert(insn);
		}
	}
}


void check_for_ret(FileIR_t* const firp, Instruction_t* const insn)
{
	assert(firp && insn);

	DISASM d;
	insn->Disassemble(d);

	if(strstr(d.Instruction.Mnemonic, "ret")==NULL)
		return;

	insn->SetIBTargets(hellnode_tgts);
}

void check_for_indirect_jmp(FileIR_t* const firp, Instruction_t* const insn)
{
	assert(firp && insn);

	DISASM d;
	insn->Disassemble(d);

	if(strstr(d.Instruction.Mnemonic, "jmp")==NULL)
		return;

	if(d.Argument1.ArgType&REGISTER_TYPE)
	{
		insn->SetIBTargets(hellnode_tgts);
	}
	else if(d.Argument1.ArgType&MEMORY_TYPE &&
			(!d.Argument1.ArgType&RELATIVE_))
	{
		insn->SetIBTargets(hellnode_tgts);
	}
}

void check_for_indirect_call(FileIR_t* const firp, Instruction_t* const insn)
{
	assert(firp && insn);

	DISASM d;
	insn->Disassemble(d);

	if (d.Instruction.BranchType!=CallType)
		return;
					
	if(d.Argument1.ArgType&CONSTANT_TYPE)
		return;

	insn->SetIBTargets(indirect_calls);
}


void calc_preds(FileIR_t* firp)
{
        preds.clear();
        for(
                set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
                it!=firp->GetInstructions().end();
                ++it
           )
        {
                Instruction_t* insn=*it;
                if(insn->GetTarget());
                        preds[insn->GetTarget()].insert(insn);
                if(insn->GetFallthrough());
                        preds[insn->GetFallthrough()].insert(insn);
        }
}


void fill_in_indtargs(FileIR_t* firp, exeio* elfiop, std::list<virtual_offset_t> forced_pins)
{
	if(getenv("IB_VERBOSE")!=0)
        	for(
                	set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
                	it!=firp->GetInstructions().end();
                	++it
           	)
		{
			Instruction_t* insn=*it;
			if(insn->GetIndirectBranchTargetAddress())
				cout<<"Insn at "<<insn->GetAddress()->GetVirtualOffset()<<" already has ibt "<<
					insn->GetIndirectBranchTargetAddress()->GetVirtualOffset()<<endl;
			
		}

	set<virtual_offset_t> thunk_bases;
	find_all_module_starts(firp,thunk_bases);


	// reset global vars
	bounds.clear();
	ranges.clear();
	targets.clear();

	calc_preds(firp);

#if 0
/* info gotten from EXEIO class now. */
        ::Elf64_Off sec_hdr_off, sec_off;
        ::Elf_Half secnum, strndx, secndx;
        ::Elf_Word secsize;

        /* Read ELF header */
        virtual_offset_t sec_hdr_off = elfiop->get_sections_offset();
        virtual_offset_t strndx = elfiop->get_section_name_str_index();
#endif
        int secnum = elfiop->sections.size();
	int secndx=0;

	/* look through each section and record bounds */
        for (secndx=0; secndx<secnum; secndx++)
		get_executable_bounds(firp, elfiop->sections[secndx]);

	/* look through each section and look for target possibilities */
        for (secndx=0; secndx<secnum; secndx++)
		infer_targets(firp, elfiop->sections[secndx]);

	
	std::list<virtual_offset_t>::iterator forced_iterator = forced_pins.begin();
	for (; forced_iterator != forced_pins.end(); forced_iterator++)
	{
		possible_target(*forced_iterator);
	}

	cout<<"========================================="<<endl;
	cout<<"Targets from data sections (and forces) are: " << endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass1="<<std::dec<<targets.size()<<endl;
	print_targets();
	cout<<"========================================="<<endl;

	/* look through the instructions in the program for targets */
	get_instruction_targets(firp, elfiop, thunk_bases);

	/* mark the entry point as a target */
	possible_target(elfiop->get_entry()); 


	cout<<"========================================="<<endl;
	cout<<"All targets from data+instruction sections are: " << endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass2="<<std::dec<<targets.size()<<endl;
	print_targets();
	cout<<"========================================="<<endl;

	/* Read the exception handler frame so that those indirect branches are accounted for */
	void read_ehframe(FileIR_t* firp, EXEIO::exeio* );
        read_ehframe(firp, elfiop);

	cout<<"========================================="<<endl;
	cout<<"All targets from data+instruction+eh_header sections are: " << endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass3="<<std::dec<<targets.size()<<endl;
	print_targets();
	cout<<"========================================="<<endl;


	/* now process the ranges that have exception handling */
	process_ranges(firp);
	cout<<"========================================="<<endl;
	cout<<"All targets from data+instruction+eh_header sections+eh_header_ranges are: " << endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass4="<<std::dec<<targets.size()<<endl;
	print_targets();
	cout<<"========================================="<<endl;

	/* now process the ranges that have exception handling */
	check_for_thunks(firp, thunk_bases);
	cout<<"========================================="<<endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass5="<<std::dec<<targets.size()<<endl;
	print_targets();
	cout<<"========================================="<<endl;

    	/* Add functions containing unsigned int params to the list */
    	add_num_handle_fn_watches(firp);
	/* now process the ranges that have exception handling */
	cout<<"========================================="<<endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass6="<<std::dec<<targets.size()<<endl;
	print_targets();
	cout<<"========================================="<<endl;


	//FILE* dynsymfile = popen( "$PS_READELF --dyn-syms readeh_tmp_file.exe |grep 'FUNC    GLOBAL DEFAULT'"
	//	"|grep -v 'FUNC    GLOBAL DEFAULT  UND' |sed 's/.*: *//'|cut -f1 -d' '", "r");
	FILE *dynsymfile = popen("$PS_OBJDUMP -T readeh_tmp_file.exe | $PS_GREP '^[0-9]\\+' | $PS_GREP -v UND | awk '{print $1;}' | $PS_GREP -v '^$'", "r");
	assert(dynsymfile);
	virtual_offset_t target=0;
	while( fscanf(dynsymfile, "%x", &target) != -1)
	{
		possible_target(target);
	}
	cout<<"========================================="<<endl;
	cout<<"# ATTRIBUTE total_indirect_targets_pass7="<<std::dec<<targets.size()<<endl;
	print_targets();
	cout<<"========================================="<<endl;




	/* set the IR to have some instructions marked as IB targets */
	mark_targets(firp);

	icfs_set_indirect_calls(firp, indirect_calls);
	icfs_set_hellnode_targets(firp, hellnode_tgts);

	mark_jmptables(firp);

	for(ICFSSet_t::const_iterator it=firp->GetAllICFS().begin();
		it != firp->GetAllICFS().end();
		++it)
	{
		ICFS_t *icfs = *it;
		cout << dec << "icfs set id: " << icfs->GetBaseID() << "  #ibtargets: " << icfs->size() << endl;
	}
}

main(int argc, char* argv[])
{
	int argc_iter = 2;

	std::list<virtual_offset_t> forced_pins;

	if(argc<2)
	{
		cerr<<"Usage: fill_in_indtargs <id> [addr,...]"<<endl;
		exit(-1);
	}

	VariantID_t *pidp=NULL;
	FileIR_t * firp=NULL;

	try 
	{
		/* setup the interface to the sql server */
		pqxxDB_t pqxx_interface;
		BaseObj_t::SetInterface(&pqxx_interface);

		pidp=new VariantID_t(atoi(argv[1]));

		for (argc_iter = 2; argc_iter < argc; argc_iter++)
		{
			char *end_ptr;
			virtual_offset_t offset = strtol(argv[argc_iter], &end_ptr, 0);
			if (*end_ptr == '\0')
			{
				cout << "force pinning: 0x" << std::hex << offset << endl;
				forced_pins.push_back(offset);
			}
		}

		assert(pidp->IsRegistered()==true);

		cout<<"New Variant, after reading registration, is: "<<*pidp << endl;

		for(set<File_t*>::iterator it=pidp->GetFiles().begin();
			it!=pidp->GetFiles().end();
			++it)
		{
			File_t* this_file=*it;
			assert(this_file);

			cout<<"Analyzing file "<<this_file->GetURL()<<endl;

			// read the db  
			firp=new FileIR_t(*pidp, this_file);

			lookupInstruction_init(firp);
			icfs_init(firp);

			int elfoid=firp->GetFile()->GetELFOID();
		        pqxx::largeobject lo(elfoid);
        		lo.to_file(pqxx_interface.GetTransaction(),"readeh_tmp_file.exe");

			jmptables.clear();

        		EXEIO::exeio*    elfiop=new EXEIO::exeio;
        		elfiop->load((const char*)"readeh_tmp_file.exe");


		
        		EXEIO::dump::header(cout,*elfiop);
        		EXEIO::dump::section_headers(cout,*elfiop);


			// find all indirect branch targets
			fill_in_indtargs(firp, elfiop, forced_pins);
	
			// write the DB back and commit our changes 
			firp->WriteToDB();

			delete firp;
			delete indirect_calls;
			delete hellnode_tgts;
		}

		pqxx_interface.Commit();

	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	assert(firp && pidp);


	delete pidp;
}
