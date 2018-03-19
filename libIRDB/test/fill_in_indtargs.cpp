
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
#include <libIRDB-util.hpp>
#include <iostream>
#include <fstream>
#include <limits>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <regex.h>
#include <ctype.h>
#include <list>
#include <stdio.h>
#include <elf.h>

#include <exeio.h>
#include "check_thunks.hpp"
#include "fill_in_indtargs.hpp"
#include "libMEDSAnnotation.h"

using namespace libIRDB;
using namespace std;
using namespace EXEIO;
using namespace MEDS_Annotation;

/*
 * defines 
 */
#define arch_ptr_bytes() (firp->GetArchitectureBitWidth()/8)

/* 
 * global variables 
 */

//
// record the ICFS for each branch, these can come from switch tables
// 
map<Instruction_t*, ICFS_t> icfs_maps;

// the bounds of the executable sections in the pgm.
set< pair <virtual_offset_t,virtual_offset_t>  > bounds;

// the set of (possible) targets we've found.
map<virtual_offset_t,ibt_provenance_t> targets;

// the set of ranges represented by the eh_frame section, could be empty for non-elf files.
set< pair< virtual_offset_t, virtual_offset_t> > ranges;

// a way to map an instruction to its set of (direct) predecessors. 
map< Instruction_t* , InstructionSet_t > preds;

// keep track of jmp tables
map< Instruction_t*, fii_icfs > jmptables;

// a map of virtual offset -> instruction for quick access.
map<virtual_offset_t,Instruction_t*> lookupInstructionMap;

// the set of things that are partially unpinned already.
set<Instruction_t*> already_unpinned;

static long total_unpins=0;


/*
 * Forward prototypes 
 */


static void check_for_PIC_switch_table32_type2(Instruction_t* insn, DecodedInstruction_t disasm, EXEIO::exeio* elfiop, const set<virtual_offset_t>& thunk_bases);
static void check_for_PIC_switch_table32_type3(FileIR_t* firp, Instruction_t* insn, DecodedInstruction_t disasm, EXEIO::exeio* elfiop, const set<virtual_offset_t>& thunk_bases);
static void check_for_PIC_switch_table32(FileIR_t*, Instruction_t* insn, DecodedInstruction_t disasm, EXEIO::exeio* elfiop, const set<virtual_offset_t>& thunk_bases);
static void check_for_PIC_switch_table64(FileIR_t*, Instruction_t* insn, DecodedInstruction_t disasm, EXEIO::exeio* elfiop);
static void check_for_nonPIC_switch_table(FileIR_t*, Instruction_t* insn, DecodedInstruction_t disasm, EXEIO::exeio* elfiop);
static void check_for_nonPIC_switch_table_pattern2(FileIR_t*, Instruction_t* insn, DecodedInstruction_t disasm, EXEIO::exeio* elfiop);

extern void read_ehframe(FileIR_t* firp, EXEIO::exeio* );




template <class T> T MAX(T a, T b) 
{
	return a>b ? a : b;
}


/*
 * range - record a new eh_frame range into the ranges global variable.
 *   this is called from read_ehframe.
 */
void range(virtual_offset_t start, virtual_offset_t end)
{ 	
	pair<virtual_offset_t,virtual_offset_t> foo(start,end);
	ranges.insert(foo);
}


/*   
 * is_in_range - determine if an address is referenced by the eh_frame section 
 */
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

/*
 * process_range -  do nothing now -- fix calls deals with this.
 */
void process_ranges(FileIR_t* firp)
{

}

bool possible_target(virtual_offset_t p, virtual_offset_t from_addr, ibt_provenance_t prov)
{
	if(is_possible_target(p,from_addr))
	{
		if(getenv("IB_VERBOSE")!=NULL)
		{
			if(from_addr!=0)
				cout<<"Found IB target address 0x"<<std::hex<<p<<" at 0x"<<from_addr<<std::dec<<", prov="<<prov<<endl;
			else
				cout<<"Found IB target address 0x"<<std::hex<<p<<" from unknown location, prov="<<prov<<endl;
		}
		targets[p].add(prov);
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

void handle_argument(const DecodedOperand_t *arg, Instruction_t* insn, ibt_provenance_t::provtype_t pt = ibt_provenance_t::ibtp_text)
{
	if(arg->isMemory()) //  (arg->ArgType&MEMORY_TYPE) == MEMORY_TYPE ) 
	{
		if(arg->isPcrel()) // (arg->ArgType&RELATIVE_)==RELATIVE_)
		{
			assert(insn);
			assert(insn->GetAddress());
			possible_target(arg->getMemoryDisplacement() /*arg->Memory.Displacement*/+insn->GetAddress()->GetVirtualOffset()+
				insn->GetDataBits().length(), insn->GetAddress()->GetVirtualOffset(), pt);
		}
		else
		{
			possible_target(arg->getMemoryDisplacement() /* arg->Memory.Displacement*/, insn->GetAddress()->GetVirtualOffset(), pt);
		}
	}
}


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
			bool isret=targets[addr].areOnlyTheseSet(ibt_provenance_t::ibtp_ret);
			bool isprintf=targets[addr].areOnlyTheseSet(ibt_provenance_t::ibtp_stars_data|ibt_provenance_t::ibtp_texttoprintf);
			if (isret)
			{
				if(getenv("IB_VERBOSE")!=NULL)
					cout<<"Skipping pin for ret at "<<hex<<addr<<endl;
			}
			else if(isprintf)
			{
				if(getenv("IB_VERBOSE")!=NULL)
					cout<<"Skipping pin for text to printf at "<<hex<<addr<<endl;
			}
			else
			{
				AddressID_t* newaddr = new AddressID_t;
				newaddr->SetFileID(insn->GetAddress()->GetFileID());
				newaddr->SetVirtualOffset(insn->GetAddress()->GetVirtualOffset());
				
				insn->SetIndirectBranchTargetAddress(newaddr);
				firp->GetAddresses().insert(newaddr);
			}
		}
	}
}


bool CallToPrintfFollows(FileIR_t *firp, Instruction_t* insn, const string& arg_str)
{
	for(Instruction_t* ptr=insn->GetFallthrough(); ptr!=NULL; ptr=ptr->GetFallthrough())
	{
		DecodedInstruction_t d(ptr);
		// Disassemble(ptr,d);
		if(d.getMnemonic() == string("call"))
		{
			// check we have a target
			if(ptr->GetTarget()==NULL)
				return false;

			// check the target has a function 
			if(ptr->GetTarget()->GetFunction()==NULL)
				return false;

			// check if we're calling printf.
			if(ptr->GetTarget()->GetFunction()->GetName().find("printf")==string::npos)
				return false;

			// found it
			return true;
		}

		// found reference to argstring, assume it's a write and exit
		if(d.getDisassembly()/*string(d.CompleteInstr)*/.find(arg_str)!= string::npos)
			return false;
	}

	return false;
}

bool texttoprintf(FileIR_t *firp,Instruction_t* insn)
{
	string dst="";
	// note that dst is an output parameter of IsParameterWrite and an input parameter to CallFollows
	if(IsParameterWrite(firp,insn, dst) && CallToPrintfFollows(firp,insn,dst))
	{
		return true;
	}
	return false;
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
                // DISASM disasm;
		DecodedInstruction_t disasm(insn);
                virtual_offset_t instr_len = disasm.length(); // Disassemble(insn,disasm);

                assert(instr_len==insn->GetDataBits().size());

		// work for both 32- and 64-bit.
		check_for_PIC_switch_table32_type2(insn,disasm, elfiop, thunk_bases);
		check_for_PIC_switch_table32_type3(firp,insn,disasm, elfiop, thunk_bases);

		if (firp->GetArchitectureBitWidth()==32)
			check_for_PIC_switch_table32(firp, insn,disasm, elfiop, thunk_bases);
		else if (firp->GetArchitectureBitWidth()==64)
			check_for_PIC_switch_table64(firp, insn,disasm, elfiop);
		else
			assert(0);

		check_for_nonPIC_switch_table(firp, insn,disasm, elfiop);
		check_for_nonPIC_switch_table_pattern2(firp, insn,disasm, elfiop);

		/* other branches can't indicate an indirect branch target */
		if(disasm.isBranch()) // disasm.Instruction.BranchType)
			continue;

		ibt_provenance_t::provtype_t prov=0;
		if(!texttoprintf(firp,insn))
		{
			prov=ibt_provenance_t::ibtp_text;
		}
		else
		{
			cout<<"TextToPrintf analysis of '"<<disasm.getDisassembly()<<"' successful at " <<hex<<insn->GetAddress()->GetVirtualOffset()<<endl;
			prov=ibt_provenance_t::ibtp_texttoprintf;
		}
		/* otherwise, any immediate is a possible branch target */
		for(const auto& op: disasm.getOperands())
		{
			if(op.isConstant())
			{
				possible_target(op.getConstant() /* Instruction.Immediat*/ ,0, prov);
			}
		}

		for(auto i=0;i<4;i++)
		{
			if(disasm.hasOperand(i))
			{
				const auto op=disasm.getOperand(i);
				handle_argument(&op, insn, prov);
			}
		}
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



		ibt_provenance_t prov;
		if(shdr->get_name()==".init_array")
			prov=ibt_provenance_t::ibtp_initarray;
		else if(shdr->get_name()==".fini_array")
			prov=ibt_provenance_t::ibtp_finiarray;
		else if(shdr->get_name()==".got.plt")
			prov=ibt_provenance_t::ibtp_gotplt;
		else if(shdr->get_name()==".got")
			prov=ibt_provenance_t::ibtp_got;
		else if(shdr->get_name()==".dynsym")
			prov=ibt_provenance_t::ibtp_dynsym;
		else if(shdr->get_name()==".symtab")
			prov=ibt_provenance_t::ibtp_symtab;
		else if(shdr->isWriteable()) 
			prov=ibt_provenance_t::ibtp_data;
		else
			prov=ibt_provenance_t::ibtp_rodata;

		possible_target(p, i+shdr->get_address(), prov);

	}

}


void print_targets()
{
	int j=1;
	for(
		map<virtual_offset_t,ibt_provenance_t>::iterator it=targets.begin();
		it!=targets.end();
		++it, j++
	   )
	{
		virtual_offset_t target=it->first;
	
		cout<<std::hex<<target;
		if(j%10 == 0)
			cout<<endl; 
		else
			cout<<", ";
	}

	cout<<endl;
}

void print_targets_oneline()
{
	int j=1;
	for(
		map<virtual_offset_t,ibt_provenance_t>::iterator it=targets.begin();
		it!=targets.end();
		++it, j++
	   )
	{
		virtual_offset_t target=it->first;
	
		cout<<std::hex<<target<<",";
	}
	cout<<endl;
}

set<Instruction_t*> find_in_function(string needle, Function_t *haystack)
{
	regex_t preg;
	set<Instruction_t*>::const_iterator fit;
	set<Instruction_t*> found_instructions;

	assert(0 == regcomp(&preg, needle.c_str(), REG_EXTENDED));

	fit = haystack->GetInstructions().begin();
	for (; fit != haystack->GetInstructions().end(); fit++)
	{
		Instruction_t *candidate = *fit;
		//DISASM disasm;
		//Disassemble(candidate,disasm);
		DecodedInstruction_t disasm(candidate);

		// check it's the requested type
		if(regexec(&preg, disasm.getDisassembly().c_str() /*CompleteInstr*/, 0, NULL, 0) == 0)
		{
			found_instructions.insert(candidate);
		}
	}
	regfree(&preg);
	return found_instructions;
}

bool backup_until(const char* insn_type_regex, Instruction_t *& prev, Instruction_t* orig, bool recursive=false)
{
	prev=orig;
	regex_t preg;

	assert(0 == regcomp(&preg, insn_type_regex, REG_EXTENDED));

	int max=10000;

	while(preds[prev].size()==1 && max-- > 0)
	{
		// get the only item in the list.
		prev=*(preds[prev].begin());
	

       		// get I7's disassembly
		//DISASM disasm;
       		//Disassemble(prev,disasm);
		DecodedInstruction_t disasm(prev);

       		// check it's the requested type
       		if(regexec(&preg, disasm.getDisassembly().c_str() /*CompleteInstr*/, 0, NULL, 0) == 0)
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
			//Disassemble(pred,disasm);
			DecodedInstruction_t disasm(pred);
       			// check it's the requested type
       			if(regexec(&preg, disasm.getDisassembly().c_str()/*CompleteInstr*/, 0, NULL, 0) == 0)
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
static void check_for_PIC_switch_table32(FileIR_t *firp, Instruction_t* insn, DecodedInstruction_t disasm, EXEIO::exeio* elfiop, const set<virtual_offset_t> &thunk_bases)
{

	ibt_provenance_t prov=ibt_provenance_t::ibtp_switchtable_type1;
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
        if(strstr(disasm.getMnemonic().c_str() /*Instruction.Mnemonic*/, "jmp")==NULL)
		return;

	// return if it's a jump to a constant address, these are common
        if(disasm.getOperand(0).isConstant() /*disasm.Argument1.ArgType&CONSTANT_TYPE*/)
		return;

	// return if it's a jump to a memory address
        if(disasm.getOperand(0).isMemory() /*disasm.Argument1.ArgType&MEMORY_TYPE*/)
		return;

	// has to be a jump to a register now

	// backup and find the instruction that's an add before I8 
	if(!backup_until("add", I4, I5))
		return;

	// backup and find the instruction that's an movsxd before I7
	if(!backup_until("mov", I3, I4))
		return;

	auto table_size = 0U;
	if (!backup_until("cmp", Icmp, I3))
	{
		cerr<<"pic32: could not find size of switch table"<<endl;
		table_size=std::numeric_limits<int>::max();
		// set table_size to be very large, so we can still do pinning appropriately
	}
	else
	{
		//DISASM dcmp;
		//Disassemble(Icmp,dcmp);
		DecodedInstruction_t dcmp(Icmp);	
		table_size = dcmp.getImmediate(); //Instruction.Immediat;
		if(table_size<=0)
			table_size=std::numeric_limits<int>::max();
	}

	// grab the offset out of the lea.
	//DISASM d2;
	//Disassemble(I3,d2);
	DecodedInstruction_t d2(I3);

	// get the offset from the thunk
	virtual_offset_t table_offset=d2.getAddress(); // d2.Instruction.AddrValue;
	if(table_offset==0)
		return;

	cout<<hex<<"Found switch dispatch at "<<I3->GetAddress()->GetVirtualOffset()<< " with table_offset="<<table_offset
	    <<" and table_size="<<table_size<<endl;
		
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
                	if((int)(offset+i*4+sizeof(int)) > (int)pSec->get_size())
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
                		if((int)(offset+i*4+sizeof(int)) > (int)pSec->get_size())
                        		break;
	
                		const int32_t *table_entry_ptr=(const int32_t*)&(secdata[offset+i*4]);
                		virtual_offset_t table_entry=*table_entry_ptr;
	
				if(getenv("IB_VERBOSE")!=0)
					cout<<"Found switch table (thunk-relative) entry["<<dec<<i<<"], "<<hex<<thunk_base+table_entry<<endl;

				if(!possible_target(thunk_base+table_entry,table_base+i*4,prov))
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
				cout << "pic32 (base pattern): valid switch table detected ibtp_switchtable_type1" << endl;
				jmptables[I5].SetTargets(ibtargets);
				jmptables[I5].SetAnalysisStatus(ICFS_Analysis_Complete);
			
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

static void check_for_PIC_switch_table32_type2(Instruction_t* insn, DecodedInstruction_t disasm, EXEIO::exeio* elfiop, const set<virtual_offset_t> &thunk_bases)
{
	ibt_provenance_t prov=ibt_provenance_t::ibtp_switchtable_type2;
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
        if(strstr(disasm.getMnemonic().c_str() /*disasm.Instruction.Mnemonic*/, "jmp")==NULL)
		return;

	// return if it's a jump to a constant address, these are common
        if(disasm.getOperand(0).isConstant() /*disasm.Argument1.ArgType&CONSTANT_TYPE*/)
		return;

	// return if it's a jump to a memory address
        //if(disasm.Argument1.ArgType&MEMORY_TYPE)
        if(disasm.getOperand(0).isMemory())
		return;

	// has to be a jump to a register now

	// backup and find the instruction that's an add before I8 
	if(!backup_until("add", I4, I5))
		return;

	// backup and find the instruction that's an movsxd before I7
	if(!backup_until("lea", I3, I4))
		return;

	// grab the offset out of the lea.
	//DISASM d2;
	//Disassemble(I3,d2);
	DecodedInstruction_t d2(I3);

	// get the offset from the thunk
	virtual_offset_t table_offset=d2.getAddress(); // d2.Instruction.AddrValue;
	if(table_offset==0)
		return;

cout<<hex<<"Found (type2) switch dispatch at "<<I3->GetAddress()->GetVirtualOffset()<< " with table_offset="<<table_offset<<endl;
		
	/* iterate over all thunk_bases/module_starts */
	for(set<virtual_offset_t>::iterator it=thunk_bases.begin(); it!=thunk_bases.end(); ++it)
	{
		//virtual_offset_t thunk_base=*it;
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
                	if((int)(offset+i*4+sizeof(int)) > (int)pSec->get_size())
                        	break;

                	const int32_t *table_entry_ptr=(const int32_t*)&(secdata[offset+i*4]);
                	virtual_offset_t table_entry=*table_entry_ptr;

// cout<<"Checking target base:" << std::hex << table_base+table_entry << ", " << table_base+i*4<<endl;
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
                		if((int)(offset+i*4+sizeof(int)) > (int)pSec->get_size())
                        		break;
	
                		const int32_t *table_entry_ptr=(const int32_t*)&(secdata[offset+i*4]);
                		virtual_offset_t table_entry=*table_entry_ptr;
	
				if(getenv("IB_VERBOSE")!=0)
					cout<<"Found switch table (thunk-relative) entry["<<dec<<i<<"], "<<hex<<table_base+table_entry<<endl;
				if(!possible_target(table_base+table_entry,table_base+i*4,prov))
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


/*
 * Detects this type of switch:
 *
 * 	  0x809900e <text_handler+51>: jmp    [eax*4 + 0x8088208]
 *
 * nb: also works for 64-bit.
 */
static void check_for_PIC_switch_table32_type3(FileIR_t* firp, Instruction_t* insn, DecodedInstruction_t disasm, EXEIO::exeio* elfiop, const set<virtual_offset_t> &thunk_bases)
{
	uint32_t ptrsize=firp->GetArchitectureBitWidth()/8;
	ibt_provenance_t prov=ibt_provenance_t::ibtp_switchtable_type3;

        Instruction_t* I5=insn;
        // check if I5 is a jump
        if(strstr(disasm.getMnemonic().c_str()/*Instruction.Mnemonic*/, "jmp")==NULL)
		return;

	// return if it's not a jump to a memory address
        if(!(disasm.getOperand(0).isMemory()))
		return;

	/* return if there's no displacement */
        if(disasm.getOperand(0).getMemoryDisplacement()==0)
		return;

        if(!disasm.getOperand(0).hasIndexRegister() || disasm.getOperand(0).getScaleValue()!=ptrsize)
		return;

	// grab the table base out of the jmp.
	virtual_offset_t table_base=disasm.getOperand(0).getMemoryDisplacement();//disasm.Argument1.Memory.Displacement;
        if(disasm.getOperand(0).isPcrel())
		table_base+=insn->GetDataBits().size()+insn->GetAddress()->GetVirtualOffset();

	if(table_base==0)
		return;

	// find the section with the data table
	EXEIO::section *pSec=find_section(table_base,elfiop);
	if(!pSec)
		return;

	auto table_max=numeric_limits<uint32_t>::max();
	auto cmp_insn=(Instruction_t*)NULL;
	if(backup_until("cmp ", cmp_insn, insn))
	{
		assert(cmp_insn);
		const auto cmp_decode=DecodedInstruction_t(cmp_insn);
		table_max=cmp_decode.getImmediate();
	}
	

	// if the section has no data, abort 
	const char* secdata=pSec->get_data();
	if(!secdata)
		return;

	// get the base offset into the section
	virtual_offset_t offset=table_base-pSec->get_address();


	// check to see if stars already marked this complete.
	// if so, just figure out the table size
	if(jmptables[insn].GetAnalysisStatus()==ICFS_Analysis_Complete)
	{
		// we already know it's a type3, we can just record the type and table base.
		jmptables[insn].AddSwitchType(prov);
		jmptables[insn].SetTableStart(table_base);

		// try to determine the table size using the complete set of targets.
		int i=0;
		for(i=0;true;i++)
		{
			
			if((int)(offset+i*ptrsize+ptrsize) > (int)pSec->get_size())
				return;

			const void *table_entry_ptr=(const int*)&(secdata[offset+i*ptrsize]);

			virtual_offset_t table_entry=0;
			switch(ptrsize)
			{
				case 4:
					table_entry=(virtual_offset_t)*(int*)table_entry_ptr;
					break;
				case 8:
					table_entry=(virtual_offset_t)*(int**)table_entry_ptr;
					break;
				default:
					assert(0);
			}

			Instruction_t* ibt=lookupInstruction(firp,table_entry);
			// if we didn't find an instruction or the insn isn't in our set, stop looking, we've found the table size
			if(ibt==NULL || jmptables[insn].find(ibt) == jmptables[insn].end())
				break;
		}
		jmptables[insn].SetTableSize(i);
		if(getenv("IB_VERBOSE"))
		{
			cout<<"Found type3 at "<<hex<<insn->GetAddress()->GetVirtualOffset()
			    <<" already complete, setting base to "<<hex<<table_base<<" and size to "<<dec<<i<<endl;
		}
		return;
	}
	

	auto i=0U;
	for(i=0;i<3;i++)
	{
		if((int)(offset+i*ptrsize+ptrsize) > (int)pSec->get_size())
			return;

		const void *table_entry_ptr=(const int*)&(secdata[offset+i*ptrsize]);

		virtual_offset_t table_entry=0;
		switch(ptrsize)
		{
			case 4:
				table_entry=(virtual_offset_t)*(int*)table_entry_ptr;
				break;
			case 8:
				table_entry=(virtual_offset_t)*(int**)table_entry_ptr;
				break;
			default:
				assert(0);
		}

		if(getenv("IB_VERBOSE")!=0)
			cout<<"Checking target base:" << std::hex << table_entry << ", " << table_base+i*ptrsize<<endl;

		/* if there's no base register and no index reg, */
		/* then this jmp can't have more than one valid table entry */
		//if( disasm.Argument1.Memory.BaseRegister==0 && disasm.Argument1.Memory.IndexRegister==0 ) 
		if( !disasm.getOperand(0).hasBaseRegister() && !disasm.getOperand(0).hasIndexRegister()) 
		{
			/* but the table can have 1 valid entry. */
			if(pSec->get_name()==".got.plt")
			{	
	                        Instruction_t *ibtarget = lookupInstruction(firp, table_entry);
				if(ibtarget)
				{
					jmptables[I5].insert(ibtarget);
					jmptables[I5].SetAnalysisStatus(ICFS_Analysis_Module_Complete);
					possible_target(table_entry,table_base+0*ptrsize, ibt_provenance_t::ibtp_gotplt);
					if(getenv("IB_VERBOSE")!=0)
						cout<<hex<<"Found  plt dispatch ("<<disasm.getDisassembly()<<"') at "<<I5->GetAddress()->GetVirtualOffset()<< endl;
					return;
				}
			}
			if(pSec->isWriteable())
				possible_target(table_entry,table_base+0*ptrsize, ibt_provenance_t::ibtp_data);
			else
				possible_target(table_entry,table_base+0*ptrsize, ibt_provenance_t::ibtp_rodata);
			if(getenv("IB_VERBOSE")!=0)
				cout<<hex<<"Found  constant-memory dispatch from non- .got.plt location ("<<disasm.getDisassembly()<<"') at "
				    <<I5->GetAddress()->GetVirtualOffset()<< endl;
			return;
		}
		if(!is_possible_target(table_entry,table_base+i*ptrsize))
		{
			if(getenv("IB_VERBOSE")!=0)
			{
				cout<<hex<<"Found (type3) candidate for switch dispatch for '"<<disasm.getDisassembly()<<"' at "
				    <<I5->GetAddress()->GetVirtualOffset()<< " with table_base="<<table_base<<endl;
				cout<<"Found table_entry "<<hex<<table_entry<<" is not valid\n"<<endl;
			}
			return;	
		}
	}


	cout<<hex<<"Definitely found (type3) switch dispatch at "<<I5->GetAddress()->GetVirtualOffset()<< " with table_base="<<table_base<<endl;

	/* did we finish the loop or break out? */
	if(i==3)
	{
		if(getenv("IB_VERBOSE")!=0)
			cout<<"Found switch table (type3)  at "<<hex<<table_base<<endl;
		jmptables[insn].AddSwitchType(prov);
		jmptables[insn].SetTableStart(table_base);
		// finished the loop.
		for(i=0;true;i++)
		{
			if(i>table_max)
			{
				if(getenv("IB_VERBOSE")!=0)
				{
					cout<<hex<<"Switch dispatch at "<<I5->GetAddress()->GetVirtualOffset()<< " with table_base="
					    <<table_base<<" is complete!"<<endl;
				}
				jmptables[insn].SetAnalysisStatus(ICFS_Analysis_Complete);
			}
			if((int)(offset+i*ptrsize+ptrsize) > (int)pSec->get_size() || i > table_max)
				return;

			const void *table_entry_ptr=(const int*)&(secdata[offset+i*ptrsize]);

			virtual_offset_t table_entry=0;
			switch(ptrsize)
			{
				case 4:
					table_entry=(virtual_offset_t)*(int*)table_entry_ptr;
					break;
				case 8:
					table_entry=(virtual_offset_t)*(int**)table_entry_ptr;
					break;
				default:
					assert(0);
			}

			Instruction_t* ibt=lookupInstruction(firp,table_entry);
			if(!possible_target(table_entry,table_base+i*ptrsize,prov) || ibt==NULL)
				return;
			if(getenv("IB_VERBOSE")!=0)
				cout<<"Found switch table (thunk-relative) entry["<<dec<<i<<"], "<<hex<<table_entry<<endl;

			// make table bigger.
			jmptables[insn].SetTableSize(i+1);/* 0 index, and this index is determined valid. */
			jmptables[insn].insert(ibt);
		}
	}
	else
	{
		if(getenv("IB_VERBOSE")!=0)
			cout<<"Found that  "<<hex<<table_base<<endl;
	}
}



/* check if this instruction is an indirect jump via a register,
 * if so, see if we can trace back a few instructions to find a
 * the start of the table.
 */
static void check_for_PIC_switch_table64(FileIR_t* firp, Instruction_t* insn, DecodedInstruction_t disasm, EXEIO::exeio* elfiop)
{
	ibt_provenance_t prov=ibt_provenance_t::ibtp_switchtable_type4;
/* here's the pattern we're looking for */
#if 0
I1:   0x000000000044425a <+218>:        cmp    DWORD PTR [rax+0x8],0xd   // bounds checking code, 0xd cases. switch(i) has i stored in [rax+8] in this e.g.
I2:   0x000000000044425e <+222>:        jbe    0x444320 <_gedit_tab_get_icon+416>
<new bb>
I3:   0x0000000000444264 <+228>:        mov    rdi,rbp // default case, also jumped to via indirect branch below
<snip (doesnt fall through)>
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




Alternate version:

   	0x00000000000b939d <+25>:	cmp    DWORD PTR [rbp-0x4],0xf
   	0x00000000000b93a1 <+29>:	ja     0xb94a1 <worker_query+285>
   	0x00000000000b93a7 <+35>:	mov    eax,DWORD PTR [rbp-0x4]
   	0x00000000000b93aa <+38>:	lea    rdx,[rax*4+0x0]
I5-1   	0x00000000000b93b2 <+46>:	lea    rax,[rip+0x1f347]        # 0xd8700
I6-2    0x00000000000b93b9 <+53>:	mov    eax,DWORD PTR [rdx+rax*1] 
I6      0x00000000000b93bc <+56>:	movsxd rdx,eax
I5-2   	0x00000000000b93bf <+59>:	lea    rax,[rip+0x1f33a]        # 0xd8700
   	0x00000000000b93c6 <+66>:	add    rax,rdx
   	0x00000000000b93c9 <+69>:	jmp    rax

Note: Since I6 doesnt access memory, do another backup until with to verify address format 

Alternate version 2:

	   0xdcf7 <+7>:	mov    r8,QWORD PTR [rdi+0xa0]
	   0xdcfe <+14>:	cmp    r8,rax
	   0xdd01 <+17>:	jbe    0xdd5c <httpd_got_request+108>
	   0xdd03 <+19>:	mov    r9,QWORD PTR [rdi+0x90]
I5	   0xdd0a <+26>:	lea    rcx,[rip+0x2a427]        # 0x38138
	   0xdd11 <+33>:	cmp    DWORD PTR [rdi+0xb0],0xb
	   0xdd18 <+40>:	movzx  edx,BYTE PTR [r9+rax*1]
	   0xdd1d <+45>:	ja     0xdd4c <httpd_got_request+92>
	   0xdd1f <+47>:	mov    esi,DWORD PTR [rdi+0xb0]
I6	   0xdd25 <+53>:	movsxd rsi,DWORD PTR [rcx+rsi*4]
I7	   0xdd29 <+57>:	add    rsi,rcx
I8	   0xdd2c <+60>:	jmp    rsi

Note: Here the operands of the add are reversed, so lookup code was not finding I5 where it was expected.


#endif


	// for now, only trying to find I4-I8.  ideally finding I1 would let us know the size of the
	// jump table.  We'll figure out N by trying targets until they fail to produce something valid.

	string table_index_str;
	Instruction_t* I8=insn;
	Instruction_t* I7=NULL;
	Instruction_t* I6=NULL;
	Instruction_t* I5=NULL;
	Instruction_t* I1=NULL;
	// check if I8 is a jump
	if(strstr(disasm.getMnemonic().c_str()/*Instruction.Mnemonic*/, "jmp")==NULL)
		return;

	// return if it's a jump to a constant address, these are common
	if(disasm.getOperand(0).isConstant()) //disasm.Argument1.ArgType&CONSTANT_TYPE)
		return;
	// return if it's a jump to a memory address
	if(disasm.getOperand(0).isMemory()) // Argument1.ArgType&MEMORY_TYPE)
		return;

	// has to be a jump to a register now

	/* 
	 * This is the instruction that adds the table value
	 * to the base address of the table. The result is 
	 * the target address of the jump.
	 *
	 * Backup and find the instruction that's an add or lea before I8.
	 */
	table_index_str = "(add ";
	table_index_str += disasm.getOperand(0).getString(); //Argument1.ArgMnemonic;
	table_index_str += "|lea ";
	table_index_str += disasm.getOperand(0).getString(); //Argument1.ArgMnemonic;
	table_index_str += ")";

	const auto cmp_str = string("cmp ") + disasm.getOperand(0).getString(); //Argument1.ArgMnemonic;

	// this was completely broken because argument2 had a null mnemonic, which we found out because getOperand(1) threw an exception.
	// i suspect it's attempting to find a compare of operand1 on the RHS of a compare, but i need better regex foo to get that.
	// for now, repeat what was working.
	const auto cmp_str2 = string("cmp "); // + disasm.getOperand(1).getString(); //Argument2.ArgMnemonic;

	if(!backup_until(table_index_str.c_str(), I7, I8))
		return;

	// Disassemble(I7,disasm);
	disasm=DecodedInstruction_t(I7);

	// Check if lea instruction is being used as add (scale=1, disp=0)
	if(strstr(disasm.getMnemonic().c_str() /* Instruction.Mnemonic*/, "lea"))
	{
		if(!(disasm.getOperand(1).isMemory() /*disasm.Argument2.ArgType&MEMORY_TYPE)*/))
			return;
		if(!(disasm.getOperand(1).getScaleValue() /* .Argument2.Memory.Scale */ == 1 && disasm.getOperand(1).getMemoryDisplacement() /*Argument2.Memory.Displacement*/ == 0))
			return;
	} 
	// backup and find the instruction that's an movsxd before I7
	/*
	 * This instruction will contain the register names for
	 * the index and the address of the base of the table
	 */
	if(!backup_until("movsxd", I6, I7))
		return;

	string lea_string="lea ";
	
	// Disassemble(I6,disasm);
	disasm=DecodedInstruction_t(I6);
	if( disasm.getOperand(1).isMemory() /* (disasm.Argument2.ArgType&MEMORY_TYPE)	 == MEMORY_TYPE*/)
	{
		// try to be smarter for memory types.

		// 64-bit register names are OK here, because this pattern already won't apply to 32-bit code.
		/*
		 * base_reg is the register that holds the address
		 * for the base of the jump table.
		 */
		string base_reg="";
		if(!disasm.getOperand(1).hasBaseRegister() /*Argument2.Memory.BaseRegister*/)
			return;
		switch(disasm.getOperand(1).getBaseRegister() /*Argument2.Memory.BaseRegister*/)
		{
			case 0/*REG0*/: base_reg="rax"; break;
			case 1/*REG1*/: base_reg="rcx"; break;
			case 2/*REG2*/: base_reg="rdx"; break;
			case 3/*REG3*/: base_reg="rbx"; break;
			case 4/*REG4*/: base_reg="rsp"; break;
			case 5/*REG5*/: base_reg="rbp"; break;
			case 6/*REG6*/: base_reg="rsi"; break;
			case 7/*REG7*/: base_reg="rdi"; break;
			case 8/*REG8*/: base_reg="r8"; break;
			case 9/*REG9*/: base_reg="r9"; break;
			case 10/*REG10*/: base_reg="r10"; break;
			case 11/*REG11*/: base_reg="r11"; break;
			case 12/*REG12*/: base_reg="r12"; break;
			case 13/*REG13*/: base_reg="r13"; break;
			case 14/*REG14*/: base_reg="r14"; break;
			case 15/*REG15*/: base_reg="r15"; break;
			default: 
				// no base register;
				return;
		}
		lea_string+=base_reg;
	}	

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
	for (; found_leas_it != found_leas.end(); found_leas_it++) 
	{
		Instruction_t *I5_cur = *found_leas_it;
		//Disassemble(I5_cur,disasm);
		disasm=DecodedInstruction_t(I5_cur);

		if(!(disasm.getOperand(1).isMemory() /*Argument2.ArgType&MEMORY_TYPE*/))
			//return;
			continue;
		if(!(disasm.getOperand(1).isPcrel() /*Argument2.ArgType&RELATIVE_*/))
			//return;
			continue;

		// note that we'd normally have to add the displacement to the
		// instruction address (and include the instruction's size, etc.
		// but, fix_calls has already removed this oddity so we can relocate
		// the instruction.
		virtual_offset_t D1=strtol(disasm.getOperand(1).getString().c_str()/*Argument2.ArgMnemonic*/, NULL, 16);

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

		auto table_size = 0U;
		if(backup_until(cmp_str.c_str(), I1, I8))
		{
			//DISASM d1;
			//Disassemble(I1,d1);
			DecodedInstruction_t d1(I1);
			table_size = d1.getImmediate(); // Instruction.Immediat;
			if (table_size <= 0)
			{
				cout<<"pic64: found I1 ('"<<d1.getDisassembly()/*CompleteInstr*/<<"'), but could not find size of switch table"<<endl;
				// set table_size to be very large, so we can still do pinning appropriately
				table_size=std::numeric_limits<int>::max();
			}
		}
		else if(backup_until(cmp_str2.c_str(), I1, I8))
		{
			//DISASM d1;
			//Disassemble(I1,d1);
			DecodedInstruction_t d1(I1);
			table_size = d1.getImmediate()/*Instruction.Immediat*/;
			if (table_size <= 0)
			{
				// set table_size to be very large, so we can still do pinning appropriately
				cout<<"pic64: found I1 ('"<<d1.getDisassembly()/*CompleteInstr*/<<"'), but could not find size of switch table"<<endl;
				table_size=std::numeric_limits<int>::max();
			}
		}
		else
		{
			cout<<"pic64: could not find size of switch table"<<endl;

			// we set the table_size variable to max_int so that we can still do pinning, 
			// but we won't do the switch identification.
			table_size=std::numeric_limits<int>::max();
		}

		set<Instruction_t *> ibtargets;
		virtual_offset_t offset=D1-pSec->get_address();
		auto entry=0U;
		auto found_table_error = false;
		do
		{
			// check that we can still grab a word from this section
			if((int)(offset+sizeof(int)) > (int)pSec->get_size())
			{
				found_table_error = true;
				break;
			}

			const int *table_entry_ptr=(const int*)&(secdata[offset]);
			virtual_offset_t table_entry=*table_entry_ptr;

			if(!possible_target(D1+table_entry, 0/* from addr unknown */,prov))
			{
				found_table_error = true;
				break;
			}

			if(getenv("IB_VERBOSE"))
			{
				cout<<"Found possible table entry, at: "<< std::hex << I8->GetAddress()->GetVirtualOffset()
			    		<< " insn: " << disasm.getDisassembly()/*CompleteInstr*/<< " d1: "
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

				found_table_error = true;
				break;
			}
			offset+=sizeof(int);
			entry++;
		} while ( entry<=table_size);

		
		// valid switch table? may or may not have default: in the switch
		// table size = 8, #entries: 9 b/c of default
		cout << "pic64: detected table size (max_int means no found): 0x"<< hex << table_size << " #entries: 0x" << entry << " ibtargets.size: " << ibtargets.size() << endl;

		// note that there may be an off-by-one error here as table size depends on whether instruction I2 is a jb or jbe.
		if (!found_table_error)
		{
			cout << "pic64: valid switch table detected ibtp_switchtable_type4" << endl;
			jmptables[I8].SetTargets(ibtargets);
			jmptables[I8].SetAnalysisStatus(ICFS_Analysis_Complete);
		}
		else
		{
			cout << "pic64: INVALID switch table detected ibtp_switchtable_type4" << endl;
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
static void check_for_nonPIC_switch_table_pattern2(FileIR_t* firp, Instruction_t* insn, DecodedInstruction_t disasm, EXEIO::exeio* elfiop)
{
	ibt_provenance_t prov=ibt_provenance_t::ibtp_switchtable_type5;
	Instruction_t *I1 = NULL;
	Instruction_t *IJ = insn;

	assert(IJ);

	// check if IJ is a jump
	//if(strstr(disasm.Instruction.Mnemonic, "jmp")==NULL)
	if(strstr(disasm.getMnemonic().c_str(), "jmp")==NULL)
		return;

	// look for a memory type
	//if(!(disasm.Argument1.ArgType&MEMORY_TYPE))
	if(!(disasm.getOperand(0).isMemory()))
		return;

	// make sure there's a scaling factor
	//if (disasm.Argument1.Memory.Scale < 4)
	//if (disasm.getOperand(0).getScaleValue() < 4) assumes scale is meaningful here?  
	if (!disasm.getOperand(0).hasIndexRegister() || disasm.getOperand(0).getScaleValue() < 4)
		return;

	// extract start of jmp table
	virtual_offset_t table_offset = disasm.getAddress(); // disasm.Instruction.AddrValue;
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
	//DISASM d1;
	//Disassemble(I1,d1);
	DecodedInstruction_t d1(I1);
	virtual_offset_t table_size = d1.getImmediate()/*d1.Instruction.Immediat*/;

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
	auto i=0U;

	set<Instruction_t*> ibtargets;
	for(i=0;i<table_size;++i)
	{
		if((int)(offset+i*arch_ptr_bytes()+sizeof(int)) > (int)pSec->get_size())
		{
			cout << "jmp table outside of section range ==> invalid switch table" << endl;
			return;
		}

		const virtual_offset_t *table_entry_ptr=(const virtual_offset_t*)&(secdata[offset+i*arch_ptr_bytes()]);
		virtual_offset_t table_entry=*table_entry_ptr;
		possible_target(table_entry,0,prov);

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

	cout << "(non-PIC) valid switch table found - ibtp_switchtable_type5" << endl;

	jmptables[IJ].SetTargets(ibtargets);
	jmptables[IJ].SetAnalysisStatus(ICFS_Analysis_Complete);
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
static void check_for_nonPIC_switch_table(FileIR_t* firp, Instruction_t* insn, DecodedInstruction_t disasm, EXEIO::exeio* elfiop)
{
	ibt_provenance_t prov=ibt_provenance_t::ibtp_switchtable_type6;
	Instruction_t *I1 = NULL;
	//Instruction_t *I2 = NULL;
	Instruction_t *I4 = NULL;
	Instruction_t *IJ = insn;

	if (!IJ) return;

	// check if IJ is a jump
	if(strstr(disasm.getMnemonic().c_str()/*disasm.Instruction.Mnemonic*/, "jmp")==NULL)
		return;

	// return if it's a jump to a constant address, these are common
	if(disasm.getOperand(0).isConstant() /*disasm.Argument1.ArgType&CONSTANT_TYPE*/)
		return;

	// return if it's a jump to a memory address
	if(disasm.getOperand(0).isMemory() /*disasm.Argument1.ArgType&MEMORY_TYPE*/)
		return;

	// has to be a jump to a register now

	// backup and find the instruction that's a mov
	if(!backup_until("mov", I4, IJ))
		return;

	// extract start of jmp table
	// DISASM d4;
	// Disassemble(I4,d4);
	DecodedInstruction_t d4(I4);

	// make sure there's a scaling factor
	if (d4.getOperand(1).isMemory() && d4.getOperand(1).getScaleValue() /*d4.Argument2.Memory.Scale*/ < 4)
		return;

	virtual_offset_t table_offset=d4.getAddress(); // d4.Instruction.AddrValue;
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
	//DISASM d1;
	//Disassemble(I1,d1);
	DecodedInstruction_t d1(I1);
	virtual_offset_t table_size = d1.getImmediate(); // d1.Instruction.Immediat;
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
	auto i=0U;

	if(getenv("IB_VERBOSE"))
		cout << hex << "offset: " << offset << " arch bit width: " << dec << firp->GetArchitectureBitWidth() << endl;

	set<Instruction_t*> ibtargets;
	for(i=0;i<table_size;++i)
	{
		if((int)(offset+i*arch_ptr_bytes()+sizeof(int)) > (int)pSec->get_size())
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

		possible_target(table_entry, 0 /* from addr unknown */, prov);
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

	cout << "(non-PIC) valid switch table found - prov=ibt_provenance_t::ibtp_switchtable_type6" << endl;
	jmptables[IJ].SetTargets(ibtargets);
	jmptables[IJ].SetAnalysisStatus(ICFS_Analysis_Complete);
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


void handle_ib_annot(FileIR_t* firp,Instruction_t* insn, MEDS_IBAnnotation* p_ib_annotation)
{
	if(p_ib_annotation->IsComplete())
	{
		jmptables[insn].SetAnalysisStatus(ICFS_Analysis_Complete);
	}
}
void handle_ibt_annot(FileIR_t* firp,Instruction_t* insn, MEDS_IBTAnnotation* p_ibt_annotation)
{
/*
 * ibt_prov reason codes
 *              static const provtype_t ibtp_stars_ret=1<<11;
 *              static const provtype_t ibtp_stars_switch=1<<12;
 *              static const provtype_t ibtp_stars_data=1<<13;
 *              static const provtype_t ibtp_stars_unknown=1<<14;
 *              static const provtype_t ibtp_stars_addressed=1<<15;
 *              static const provtype_t ibtp_stars_unreachable=1<<15;
 */
/* meds annotations
 *                typedef enum { SWITCH, RET, DATA, UNREACHABLE, ADDRESSED, UNKNOWN } ibt_reason_code_t;
 */
	//cout<<"at handl_ibt with addr="<<hex<<insn->GetAddress()->GetVirtualOffset()<<" code="<<p_ibt_annotation->GetReason()<<endl;
	switch(p_ibt_annotation->GetReason())
	{
		case MEDS_IBTAnnotation::SWITCH:
		case MEDS_IBTAnnotation::INDIRCALL:
		{
			possible_target((EXEIO::virtual_offset_t)p_ibt_annotation->getVirtualOffset().getOffset(),
				0,ibt_provenance_t::ibtp_stars_switch);
			libIRDB::virtual_offset_t  addr=(libIRDB::virtual_offset_t)p_ibt_annotation->GetXrefAddr();
			Instruction_t* fromib=lookupInstruction(firp, addr);
			Instruction_t* ibt=lookupInstruction(firp, p_ibt_annotation->getVirtualOffset().getOffset());
			if(fromib && ibt)
			{
				if(getenv("IB_VERBOSE")!=NULL)
					cout<<hex<<"Adding call/switch icfs: "<<fromib->GetAddress()->GetVirtualOffset()<<"->"<<ibt->GetAddress()->GetVirtualOffset()<<endl;
				jmptables[fromib].insert(ibt);
			}
			else
			{
				cout<<"Warning:  cannot find source or dest for call/switch icfs."<<endl;
			}
			break;
		}
		case MEDS_IBTAnnotation::RET:
		{
			/* we are not going to mark return points as IBTs yet.  that's fix-calls job */
			// possible_target((EXEIO::virtual_offset_t)p_ibt_annotation->getVirtualOffset().getOffset(),
			// 	0,ibt_provenance_t::ibtp_stars_ret);


			libIRDB::virtual_offset_t  fromaddr=(libIRDB::virtual_offset_t)p_ibt_annotation->GetXrefAddr();
			Instruction_t* fromib=lookupInstruction(firp, fromaddr);
			libIRDB::virtual_offset_t  toaddr=p_ibt_annotation->getVirtualOffset().getOffset();
			Instruction_t* ibt=lookupInstruction(firp, toaddr);
			if(fromib && ibt)
			{
				if(getenv("IB_VERBOSE")!=NULL)
					cout<<hex<<"Adding ret icfs: "<<fromib->GetAddress()->GetVirtualOffset()<<"->"<<ibt->GetAddress()->GetVirtualOffset()<<endl;
				jmptables[fromib].insert(ibt);
			}
			else
			{
				cout<<"Warning:  cannot find source ("<<hex<<fromaddr<<") or dest ("<<hex<<toaddr<<") for ret icfs."<<endl;
			}
			break;
		}
		case MEDS_IBTAnnotation::DATA:
		{
			possible_target((EXEIO::virtual_offset_t)p_ibt_annotation->getVirtualOffset().getOffset(),
				0,ibt_provenance_t::ibtp_stars_data);
			if(getenv("IB_VERBOSE")!=NULL)
				cout<<hex<<"detected stars data ibt at"<<p_ibt_annotation->getVirtualOffset().getOffset()<<endl;
			break;
		}
		case MEDS_IBTAnnotation::UNREACHABLE:
		{
			possible_target((EXEIO::virtual_offset_t)p_ibt_annotation->getVirtualOffset().getOffset(),
				0,ibt_provenance_t::ibtp_stars_unreachable);
			if(getenv("IB_VERBOSE")!=NULL)
				cout<<hex<<"detected stars unreachable ibt at"<<p_ibt_annotation->getVirtualOffset().getOffset()<<endl;
			break;
		}
		case MEDS_IBTAnnotation::ADDRESSED:
		{
			possible_target((EXEIO::virtual_offset_t)p_ibt_annotation->getVirtualOffset().getOffset(),
				0,ibt_provenance_t::ibtp_stars_addressed);
			if(getenv("IB_VERBOSE")!=NULL)
				cout<<hex<<"detected stars addresssed ibt at"<<p_ibt_annotation->getVirtualOffset().getOffset()<<endl;
			break;
		}
		case MEDS_IBTAnnotation::UNKNOWN:
		{
			possible_target((EXEIO::virtual_offset_t)p_ibt_annotation->getVirtualOffset().getOffset(),
				0,ibt_provenance_t::ibtp_stars_unknown);
			if(getenv("IB_VERBOSE")!=NULL)
				cout<<hex<<"detected stars unknown ibt at"<<p_ibt_annotation->getVirtualOffset().getOffset()<<endl;
			break;
		}
		default:
		{
			assert(0); // unexpected ibt annotation.
		}
	}


}

void read_stars_xref_file(FileIR_t* firp)
{

	string BINARY_NAME="a.ncexe";
	string SHARED_OBJECTS_DIR="shared_objects";

	string fileBasename = basename((char*)firp->GetFile()->GetURL().c_str());
	int ibs=0;
	int ibts=0;

	MEDS_AnnotationParser annotationParser;
	string annotationFilename;
	// need to map filename to integer annotation file produced by STARS
	// this should be retrieved from the IRDB but for now, we use files to store annotations
	// convention from within the peasoup subdirectory is:
	//      a.ncexe.infoannot
	//      shared_objects/<shared-lib-filename>.infoannot
	if (fileBasename==BINARY_NAME) 
		annotationFilename = BINARY_NAME;
	else
		annotationFilename = SHARED_OBJECTS_DIR + "/" + fileBasename ;

	try
	{
		annotationParser.parseFile(annotationFilename+".STARSxrefs");
	}
	catch (const string &s)
	{
		cout<<"Warning:  annotation parser reports error: "<<s<<endl;
	}

        for(
                set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
                it!=firp->GetInstructions().end();
                ++it
           )
	{
		Instruction_t* insn=*it;
		virtual_offset_t irdb_vo = insn->GetAddress()->GetVirtualOffset();
		VirtualOffset vo(irdb_vo);

		/* find it in the annotations */
        	pair<MEDS_Annotations_t::iterator,MEDS_Annotations_t::iterator> ret;
		ret = annotationParser.getAnnotations().equal_range(vo);
		MEDS_IBAnnotation* p_ib_annotation;
		MEDS_IBTAnnotation* p_ibt_annotation;

		/* for each annotation for this instruction */
		for (MEDS_Annotations_t::iterator ait = ret.first; ait != ret.second; ++ait)
		{
			/* is this annotation a funcSafe annotation? */
			p_ib_annotation=dynamic_cast<MEDS_IBAnnotation*>(ait->second);
			if(p_ib_annotation && p_ib_annotation->isValid())
			{
				ibs++;
				handle_ib_annot(firp,insn,p_ib_annotation);
			}
			p_ibt_annotation=dynamic_cast<MEDS_IBTAnnotation*>(ait->second);
			if(p_ibt_annotation && p_ibt_annotation->isValid())
			{
				ibts++;
				handle_ibt_annot(firp,insn,p_ibt_annotation);
			}
		}
	}

	cout<<"Found "<<ibs<<" ibs and "<<ibts<<" ibts in the STARSxref file."<<endl;

}

void process_dynsym(FileIR_t* firp)
{
	auto dynsymfile = popen("$PS_OBJDUMP -T readeh_tmp_file.exe | $PS_GREP '^[0-9]\\+' | $PS_GREP -v UND | awk '{print $1;}' | $PS_GREP -v '^$'", "r");
	assert(dynsymfile);
	auto target=(unsigned int)0;
	while( fscanf(dynsymfile, "%x", &target) != -1)
	{
		possible_target((virtual_offset_t)target,0,ibt_provenance_t::ibtp_dynsym);
	}
}


ICFS_t* setup_hellnode(FileIR_t* firp, EXEIO::exeio* elfiop, ibt_provenance_t allowed)
{
	ICFS_t* hn=new ICFS_t(ICFS_Analysis_Module_Complete);

        for(
                set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
                it!=firp->GetInstructions().end();
                ++it
           )
	{
		Instruction_t* insn=*it;

		/*
		if(insn->GetIndirectBranchTargetAddress() == NULL)
			continue;
		*/

		ibt_provenance_t prov=targets[insn->GetAddress()->GetVirtualOffset()];

		if(prov.isEmpty())
			continue;

		if(prov.isPartiallySet(allowed))
		{
			hn->insert(insn);
		}
	}

	if(hn->size() < 1000 && !elfiop->isDynamicallyLinked())
		hn->SetAnalysisStatus(ICFS_Analysis_Complete);

	return hn;
}

ICFS_t* setup_call_hellnode(FileIR_t* firp, EXEIO::exeio* elfiop)
{
	ibt_provenance_t allowed=
		ibt_provenance_t::ibtp_data |
		ibt_provenance_t::ibtp_text |
		ibt_provenance_t::ibtp_stars_addressed |
		ibt_provenance_t::ibtp_unknown |
		ibt_provenance_t::ibtp_stars_unreachable |
	 	ibt_provenance_t::ibtp_dynsym |		// symbol resolved to other module, this module should xfer directly, but could still plt 
		ibt_provenance_t::ibtp_rodata |
	 	ibt_provenance_t::ibtp_initarray |	// .init loops through the init_array, and calls them
	 	ibt_provenance_t::ibtp_finiarray |	// .fini loops through the fini_array, and calls them
		ibt_provenance_t::ibtp_user;


// would like to sanity check better.
//		ibt_provenance_t::ibtp_stars_data |	// warn if stars reports it's in data, but !allowed.

	/*
	 * these aren't good enough reasons for a call instruction to transfer somewhere.
	 * ibt_provenance_t::ibtp_eh_frame	// only libc should xfer.
	 * ibt_provenance_t::ibtp_gotplt	// only an analyzed jump should xfer.
	 * ibt_provenance_t::ibtp_entrypoint	// only ld.so or kernel should xfer.
	 * ibt_provenance_t::ibtp_texttoprintf	// shouldn't xfer if addr passed to printf.
	 * ibt_provenance_t::ibtp_symtab		// user info only.
	 * ibt_provenance_t::ibtp_stars_ret	// stars says a return goes here, calls shouldn't.
	 * ibt_provenance_t::ibtp_stars_switch	// stars says switch target.
	 * ibt_provenance_t::ibtp_switchtable_type1	// FII switch targets.
	 * ibt_provenance_t::ibtp_switchtable_type2
	 * ibt_provenance_t::ibtp_switchtable_type3
	 * ibt_provenance_t::ibtp_switchtable_type4
	 * ibt_provenance_t::ibtp_switchtable_type5
	 * ibt_provenance_t::ibtp_switchtable_type6
	 * ibt_provenance_t::ibtp_switchtable_type7
	 * ibt_provenance_t::ibtp_switchtable_type8
	 * ibt_provenance_t::ibtp_switchtable_type9
	 * ibt_provenance_t::ibtp_switchtable_type10
	 */

	ICFS_t* ret=setup_hellnode(firp,elfiop,allowed);

	cout<<"# ATTRIBUTE fill_in_indtargs::call_hellnode_size="<<dec<<ret->size()<<endl;
	return ret;

}

ICFS_t* setup_jmp_hellnode(FileIR_t* firp, EXEIO::exeio* elfiop)
{
	ibt_provenance_t allowed=
		ibt_provenance_t::ibtp_data |
		ibt_provenance_t::ibtp_text |
		ibt_provenance_t::ibtp_stars_addressed |
		ibt_provenance_t::ibtp_unknown |
		ibt_provenance_t::ibtp_stars_unreachable |
	 	ibt_provenance_t::ibtp_dynsym |		// symbol resolved to other module, this module should xfer directly. but that's not true, may still plt.
		ibt_provenance_t::ibtp_rodata |
		ibt_provenance_t::ibtp_gotplt |
		ibt_provenance_t::ibtp_user;

//		ibt_provenance_t::ibtp_stars_data |	// warn if stars reports it's in data, but !allowed.

	/* 
	 * these aren't good enough reasons for a jmp instruction to transfer somewhere.
	 * ibt_provenance_t::ibtp_eh_frame	// only libc should xfer.
	 * ibt_provenance_t::ibtp_initarray	// only ld.so should xfer.
	 * ibt_provenance_t::ibtp_finiarray	// only ld.so should xfer.
	 * ibt_provenance_t::ibtp_entrypoint	// only ld.so or kernel should xfer.
	 * ibt_provenance_t::ibtp_texttoprintf	// shouldn't xfer if addr passed to printf.
	 * ibt_provenance_t::ibtp_symtab		// user info only.
	 * ibt_provenance_t::ibtp_stars_ret	// stars says a return goes here, calls shouldn't.
	 * ibt_provenance_t::ibtp_stars_switch	// stars says switch target.
	 * ibt_provenance_t::ibtp_switchtable_type1	// FII switch targets.
	 * ibt_provenance_t::ibtp_switchtable_type2
	 * ibt_provenance_t::ibtp_switchtable_type3
	 * ibt_provenance_t::ibtp_switchtable_type4
	 * ibt_provenance_t::ibtp_switchtable_type5
	 * ibt_provenance_t::ibtp_switchtable_type6
	 * ibt_provenance_t::ibtp_switchtable_type7
	 * ibt_provenance_t::ibtp_switchtable_type8
	 * ibt_provenance_t::ibtp_switchtable_type9
	 * ibt_provenance_t::ibtp_switchtable_type10
	 */

	ICFS_t* ret=setup_hellnode(firp,elfiop,allowed);
	cout<<"# ATTRIBUTE fill_in_indtargs::jmp_hellnode_size="<<dec<<ret->size()<<endl;
	return ret;

}


ICFS_t* setup_ret_hellnode(FileIR_t* firp, EXEIO::exeio* elfiop)
{
	ibt_provenance_t allowed=
		ibt_provenance_t::ibtp_stars_ret |	// stars says a return goes here, and this return isn't analyzeable.
		ibt_provenance_t::ibtp_unknown |
		ibt_provenance_t::ibtp_stars_unreachable |
		ibt_provenance_t::ibtp_ret |	// instruction after a call
		ibt_provenance_t::ibtp_user;


// would like to sanity check better.
//		ibt_provenance_t::ibtp_stars_data |	// warn if stars reports it's in data, but !allowed.


	/*
	 * these aren't good enough reasons for a ret instruction to transfer somewhere.
	 * ibt_provenance_t::ibtp_eh_frame	// only libc should xfer.
	 * ibt_provenance_t::ibtp_initarray	// only ld.so should xfer.
	 * ibt_provenance_t::ibtp_finiarray	// only ld.so should xfer.
	 * ibt_provenance_t::ibtp_entrypoint	// only ld.so or kernel should xfer.
	 * ibt_provenance_t::ibtp_texttoprintf	// shouldn't xfer if addr passed to printf.
	 * ibt_provenance_t::ibtp_dynsym		// symbol resolved to other module, this module should xfer directly. 
	 * ibt_provenance_t::ibtp_symtab		// user info only.
	 * ibt_provenance_t::ibtp_stars_ret	// stars says a return goes here, calls shouldn't.
	 * ibt_provenance_t::ibtp_stars_switch	// stars says switch target.
	 * ibt_provenance_t::ibtp_switchtable_type1	// FII switch targets.
	 * ibt_provenance_t::ibtp_switchtable_type2
	 * ibt_provenance_t::ibtp_switchtable_type3
	 * ibt_provenance_t::ibtp_switchtable_type4
	 * ibt_provenance_t::ibtp_switchtable_type5
	 * ibt_provenance_t::ibtp_switchtable_type6
	 * ibt_provenance_t::ibtp_switchtable_type7
	 * ibt_provenance_t::ibtp_switchtable_type8
	 * ibt_provenance_t::ibtp_switchtable_type9
	 * ibt_provenance_t::ibtp_switchtable_type10
	 * ibt_provenance_t::ibtp_data  	// returns likely shouldn't be used to jump to data or addressed text chunks.  may need to relax later.
	 * ibt_provenance_t::ibtp_text  
	 * ibt_provenance_t::ibtp_stars_addressed  
	 * ibt_provenance_t::ibtp_rodata  
	 * ibt_provenance_t::ibtp_gotplt  
	 */

	ICFS_t* ret_hell_node=setup_hellnode(firp,elfiop,allowed);
	cout<<"# ATTRIBUTE fill_in_indtargs::basicret_hellnode_size="<<dec<<ret_hell_node->size()<<endl;

	cout<<"# ATTRIBUTE fill_in_indtargs::fullret_hellnode_size="<<dec<<ret_hell_node->size()<<endl;
	return ret_hell_node;
}


void mark_return_points(FileIR_t* firp)
{

	// add unmarked return points.  fix_calls will deal with whether they need to be pinned or not later.
        for(
		InstructionSet_t::const_iterator it=firp->GetInstructions().begin();
                it!=firp->GetInstructions().end();
                ++it
           )
	{
		Instruction_t* insn=*it;
		//DISASM d;
		//Disassemble(insn,d);
		DecodedInstruction_t d(insn);
		if(string("call")==d.getMnemonic() /*.Instruction.Mnemonic*/ && insn->GetFallthrough())
		{
			targets[insn->GetFallthrough()->GetAddress()->GetVirtualOffset()].add(ibt_provenance_t::ibtp_ret);
		}
	}
}


void print_icfs(FileIR_t* firp)
{
	cout<<"Printing ICFS sets."<<endl;
        for(
		InstructionSet_t::const_iterator it=firp->GetInstructions().begin();
                it!=firp->GetInstructions().end();
                ++it
           )
	{
		Instruction_t* insn=*it;
		ICFS_t *icfs=insn->GetIBTargets();

		// not an IB
		if(!icfs)
			continue;

		cout<<hex<<insn->GetAddress()->GetVirtualOffset()<<" -> ";

		for(ICFS_t::const_iterator icfsit=icfs->begin(); icfsit!=icfs->end(); ++icfsit)
		{
			Instruction_t* target=*icfsit;
			cout<<hex<<target->GetAddress()->GetVirtualOffset()<<" ";
		}
		cout<<endl;
	}
}

void setup_icfs(FileIR_t* firp, EXEIO::exeio* elfiop)
{
	int total_ibta_set=0;

	/* setup some IBT categories for warning checking */
        ibt_provenance_t non_stars_data=
                ibt_provenance_t::ibtp_text |
                ibt_provenance_t::ibtp_eh_frame |
                ibt_provenance_t::ibtp_texttoprintf |
                ibt_provenance_t::ibtp_gotplt |
                ibt_provenance_t::ibtp_initarray |
                ibt_provenance_t::ibtp_finiarray |
                ibt_provenance_t::ibtp_data |
                ibt_provenance_t::ibtp_dynsym |
                ibt_provenance_t::ibtp_symtab |
                ibt_provenance_t::ibtp_rodata |
                ibt_provenance_t::ibtp_unknown;
        ibt_provenance_t stars_data=ibt_provenance_t::ibtp_stars_data ;



	// setup calls, jmps and ret hell nodes.
	ICFS_t *call_hell = setup_call_hellnode(firp,elfiop);
	firp->GetAllICFS().insert(call_hell);

	ICFS_t *jmp_hell = setup_jmp_hellnode(firp,elfiop);
	firp->GetAllICFS().insert(jmp_hell);

	ICFS_t *ret_hell = setup_ret_hellnode(firp,elfiop);
	firp->GetAllICFS().insert(ret_hell);


	// for each instruction 
        for(
                set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
                it!=firp->GetInstructions().end();
                ++it
           )
	{

		// if we already got it complete (via stars or FII)
		Instruction_t* insn=*it;

		if(insn->GetIndirectBranchTargetAddress()!=NULL)
			total_ibta_set++;
			

		// warning check
		ibt_provenance_t prov=targets[insn->GetAddress()->GetVirtualOffset()];

		// stars calls it data, but printw arning if we didn't find it in data or as a printf addr.
		if(prov.isPartiallySet(stars_data) && !prov.isPartiallySet(non_stars_data))
		{
			//ofstream fout("warning.txt", ofstream::out | ofstream::app);
			cerr<<"STARS found an IBT in data that FII wasn't able to classify at "<<hex<<insn->GetAddress()->GetVirtualOffset()<<"."<<endl;
		}

		// create icfs for complete jump tables.
		if(jmptables[insn].IsComplete())
		{
			if(getenv("IB_VERBOSE")!=0)
			{
				cout<<"IB complete for "<<hex<<insn->GetAddress()->GetVirtualOffset()
					<<":"<<insn->getDisassembly()<<endl;
			}
			// get the strcuture into the IRDB	
			ICFS_t* nn=new ICFS_t(jmptables[insn]);
			firp->GetAllICFS().insert(nn);
			insn->SetIBTargets(nn);

			// that's all we need to do
			continue;
		}

		// disassemble the instruction, and figure out which type of hell node we need.
		//DISASM d;
		//Disassemble(insn,d);
		DecodedInstruction_t d(insn);
		if(d.isReturn()) // string("ret")==d.getMnemonic() /*Instruction.Mnemonic*/ || string("retn")==d.getMnemonic() /*d.Instruction.Mnemonic*/)
		{
			if(getenv("IB_VERBOSE")!=0)
				cout<<"using ret hell node for "<<hex<<insn->GetAddress()->GetVirtualOffset()<<endl;
			insn->SetIBTargets(ret_hell);
		}
		//else if ( (string("call ")==d.Instruction.Mnemonic) && ((d.Argument1.ArgType&0xffff0000&CONSTANT_TYPE)!=CONSTANT_TYPE))
		else if ( d.isCall() /* (string("call")==d.getMnemonic()) */ && (!d.getOperand(0).isConstant()))
		{
			if(getenv("IB_VERBOSE")!=0)
				cout<<"using call hell node for "<<hex<<insn->GetAddress()->GetVirtualOffset()<<endl;
			// indirect call 
			insn->SetIBTargets(call_hell);
		}
		//else if ( (string("jmp ")==d.Instruction.Mnemonic) && ((d.Argument1.ArgType&0xffff0000&CONSTANT_TYPE)!=CONSTANT_TYPE))
		else if ( d.isUnconditionalBranch() /*(string("jmp")==d.getMnemonic()) */&& (!d.getOperand(0).isConstant()))
		{
			if(getenv("IB_VERBOSE")!=0)
				cout<<"using jmp hell node for "<<hex<<insn->GetAddress()->GetVirtualOffset()<<endl;
			// indirect jmp 
			insn->SetIBTargets(jmp_hell);
		}

	}

	cout<<"# ATTRIBUTE fill_in_indtargs::total_ibtas_set="<<dec<<total_ibta_set<<endl;

	if(getenv("ICFS_VERBOSE")!=NULL)
		print_icfs(firp);
}


void unpin_elf_tables(FileIR_t *firp, int64_t do_unpin_opt)
{
	map<string,int> unpin_counts;
	map<string,int> missed_unpins;

	for(
		DataScoopSet_t::iterator it=firp->GetDataScoops().begin();
		it!=firp->GetDataScoops().end();
		++it
	   )
	{
		// 4 or 8 
		int ptrsize=firp->GetArchitectureBitWidth()/8;

		DataScoop_t* scoop=*it;
		const char *scoop_contents=scoop->GetContents().c_str();
		if(scoop->GetName()==".init_array" || scoop->GetName()==".fini_array" || scoop->GetName()==".got.plt" || scoop->GetName()==".got")
		{
			auto start_offset=0U;
			if(scoop->GetName()==".got.plt")
			{
				// .got.plt has a start index of 4 pointers into the section.
				start_offset=4*ptrsize;
			}
			for(auto i=start_offset; i+ptrsize <= scoop->GetSize() ; i+=ptrsize)
			{
				virtual_offset_t vo;
				if(ptrsize==4)
					/* get int, 4 bytes */
					vo=(virtual_offset_t)*(uint32_t*)&scoop_contents[i];
				else if(ptrsize==8)
					/* get long long, 8 bytes */
					vo=(virtual_offset_t)*(uint64_t*)&scoop_contents[i];
				else
					assert(0);	


				Instruction_t* insn=lookupInstruction(firp,vo);

				// OK for .got scoop to miss
				if(scoop->GetName()==".got" && insn==NULL)
				{
					if(getenv("UNPIN_VERBOSE")!=0)
						cout<<"Skipping "<<scoop->GetName()<<" unpin for "<<hex<<vo<<" due to no instruction at vo"<<endl;
					continue;
				}

				// these asserts are probably overkill, but want them for sanity checking for now.
				assert(insn);
				assert(targets.find(vo)!=targets.end());

				if( targets[vo].areOnlyTheseSet(
					ibt_provenance_t::ibtp_initarray | 
					ibt_provenance_t::ibtp_finiarray | 
					ibt_provenance_t::ibtp_gotplt | 
					ibt_provenance_t::ibtp_got | 
					ibt_provenance_t::ibtp_stars_data)
				  )
				{
					total_unpins++;
					auto allow_unpin=true;
					if(do_unpin_opt != -1)
					{
						if(total_unpins > do_unpin_opt) 
						{
						/*
							cout<<"Aborting unpin process mid elf table."<<endl;
							return;
						*/
							allow_unpin=false;
						}
					}

					// when/if they fail, convert to if and guard the reloc creation.
					if(getenv("UNPIN_VERBOSE")!=0)
					{
						if(allow_unpin)
							cout<<"Attempting unpin #"<<total_unpins<<"."<<endl;
						else
							cout<<"Eliding unpin #"<<total_unpins<<"."<<endl;
					}

					if(allow_unpin || already_unpinned.find(insn)!=already_unpinned.end())
					{
						already_unpinned.insert(insn);
						unpin_counts[scoop->GetName()]++;

						Relocation_t* nr=new Relocation_t();
						assert(nr);
						nr->SetType("data_to_insn_ptr");
						nr->SetOffset(i);
						nr->SetWRT(insn);

						// add reloc to IR.
						firp->GetRelocations().insert(nr);
						scoop->GetRelocations().insert(nr);

						if(getenv("UNPIN_VERBOSE")!=0)
							cout<<"Unpinning "+scoop->GetName()+" entry at offset "<<dec<<i<<endl;
					}
				}
				else
				{
					if(getenv("UNPIN_VERBOSE")!=0)
						cout<<"Skipping "<<scoop->GetName()<<" unpin for "<<hex<<vo<<" due to other references at offset="<<dec<<i<<endl;
					missed_unpins[scoop->GetName()]++;
				}
			}
		}
		else if(scoop->GetName()==".dynsym")
		{
			Elf64_Sym  *sym64=NULL;
			Elf32_Sym  *sym32=NULL;
			int ptrsize=0;
			int symsize=0;
			const char* scoop_contents=scoop->GetContents().c_str();
			switch(firp->GetArchitectureBitWidth())
			{	
				case 64:
					ptrsize=8;
					symsize=sizeof(Elf64_Sym);
					break;	
				case 32:
					ptrsize=4;
					symsize=sizeof(Elf32_Sym);
					break;	
				default:
					assert(0);
				
			}
			auto table_entry_no=0U;
			for(auto i=0U;i+symsize<scoop->GetSize(); i+=symsize, table_entry_no++)
			{
				int addr_offset=0;
				virtual_offset_t vo=0;
				int st_info_field=0;
				int shndx=0;
				switch(ptrsize)
				{
					case 4:
					{
						sym32=(Elf32_Sym*)&scoop_contents[i];
						addr_offset=(uintptr_t)&(sym32->st_value)-(uintptr_t)sym32;
						vo=sym32->st_value;
						st_info_field=sym32->st_info;
						shndx=sym32->st_shndx;
						break;
					}
					case 8:
					{
						sym64=(Elf64_Sym*)&scoop_contents[i];
						addr_offset=(uintptr_t)&(sym64->st_value)-(uintptr_t)sym64;
						vo=sym64->st_value;
						st_info_field=sym64->st_info;
						shndx=sym64->st_shndx;
						break;
						break;
					}
					default:
						assert(0);
				}

				// this is good for both 32- and 64-bit.
				int type=ELF32_ST_TYPE(st_info_field);

				if(shndx!=SHN_UNDEF && type==STT_FUNC)
				{
					Instruction_t* insn=lookupInstruction(firp,vo);


					// these asserts are probably overkill, but want them for sanity checking for now.
					assert(insn);
					assert(targets.find(vo)!=targets.end());

					// check that the ibt is only ref'd by .dynsym (and STARS, which is ambigulous
					// about which section 
					if(targets[vo].areOnlyTheseSet(
						ibt_provenance_t::ibtp_dynsym | ibt_provenance_t::ibtp_stars_data))
					{
						total_unpins++;
						if(do_unpin_opt != -1)
						{
							if(total_unpins > do_unpin_opt) 
							{
								cout<<"Aborting unpin process mid elf table."<<endl;
								return;
							}
							cout<<"Attempting unpin #"<<total_unpins<<"."<<endl;
						}

						if(getenv("UNPIN_VERBOSE")!=0)
							cout<<"Unpinning .dynsym entry no "<<dec<<table_entry_no<<". vo="<<hex<<vo<<endl;


						// when/if these asserts fail, convert to if and guard the reloc creation.

						unpin_counts[scoop->GetName()]++;
						Relocation_t* nr=new Relocation_t();
						assert(nr);
						nr->SetType("data_to_insn_ptr");
						nr->SetOffset(i+addr_offset);
						nr->SetWRT(insn);

						// add reloc to IR.
						firp->GetRelocations().insert(nr);
						scoop->GetRelocations().insert(nr);
					}
					else
					{
						if(getenv("UNPIN_VERBOSE")!=0)
							cout<<"Skipping .dynsm unpin for "<<hex<<vo<<" due to other references."<<dec<<i<<endl;
						missed_unpins[scoop->GetName()]++;
					}
				}
			}
		}	
		else
		{
			if(getenv("UNPIN_VERBOSE")!=0)
				cout<<"Skipping unpin of section "<<scoop->GetName()<<endl;
		}
			
	}

	int total_elftable_unpins=0;

	// print unpin stats.
	for(map<string,int>::iterator it=unpin_counts.begin(); it!=unpin_counts.end(); ++it)
	{
		string name=it->first;
		int count=it->second;
		cout<<"# ATTRIBUTE fill_in_indtargs::unpin_count_"<<name<<"="<<dec<<count<<endl;
		total_elftable_unpins++;
	}
	for(map<string,int>::iterator it=missed_unpins.begin(); it!=missed_unpins.end(); ++it)
	{
		string name=it->first;
		int count=it->second;
		cout<<"# ATTRIBUTE fill_in_indtargs::hmissed_unpin_count_"<<name<<"="<<dec<<count<<endl;
	}
	cout<<"# ATTRIBUTE fill_in_indtargs::total_elftable_unpins="<<dec<<total_elftable_unpins<<endl;

}

DataScoop_t* find_scoop(FileIR_t *firp, const virtual_offset_t &vo)
{
	for(
		DataScoopSet_t::iterator it=firp->GetDataScoops().begin();
		it!=firp->GetDataScoops().end();
		++it
	   )
	{
		DataScoop_t* s=*it;
		if( s->GetStart()->GetVirtualOffset()<=vo && vo<s->GetEnd()->GetVirtualOffset() )
			return s;
	}
	return NULL;
}

// stats for unpinning.
int type3_unpins=0;
int type3_pins=0;

void unpin_type3_switchtable(FileIR_t* firp,Instruction_t* insn,DataScoop_t* scoop, int64_t do_unpin_opt)
{

	assert(firp && insn && scoop);

	set<Instruction_t*> switch_targs;

	if(getenv("UNPIN_VERBOSE"))
	{
		cout<<"Unpinning type3 switch, dispatch is "<<hex<<insn->GetAddress()->GetVirtualOffset()<<":"
		    <<insn->getDisassembly()<<" with tabSz="<<jmptables[insn].GetTableSize()<<endl; 
	}


	// switch check
	// could be dangerous if the IBT is found in rodata section,
	// but then also used for a switch.  this is unlikely.
	ibt_provenance_t prov=ibt_provenance_t::ibtp_switchtable_type3 |	 // found as switch
		ibt_provenance_t::ibtp_stars_switch |  	// found as stars switch
		ibt_provenance_t::ibtp_rodata;		// found in rodata.

	ibt_provenance_t newprov=ibt_provenance_t::ibtp_switchtable_type3 |	 // found as switch
		ibt_provenance_t::ibtp_stars_switch ;  	// found as stars switch

	// ptr size
	int ptrsize=firp->GetArchitectureBitWidth()/8;

	// offset from start of scoop
	virtual_offset_t scoop_off=jmptables[insn].GetTableStart() - scoop->GetStart()->GetVirtualOffset();

	// scoop contents
	const char *scoop_contents=scoop->GetContents().c_str();


	for(int i=0; i<jmptables[insn].GetTableSize(); i++)
	{

		// grab the value out of the scoop
		virtual_offset_t table_entry=0;		
		switch(ptrsize)
		{
			case 4:
				table_entry=(virtual_offset_t)*(int*)&scoop_contents[scoop_off];
				break;
			case 8:
				table_entry=(virtual_offset_t)*(int**)&scoop_contents[scoop_off];
				break;
			default:
				assert(0);
		}

		// verify we have an instruction.
		Instruction_t* ibt=lookupInstruction(firp,table_entry);
		if(ibt)
		{
			// which isn't otherwise addressed.
			if(targets[table_entry].areOnlyTheseSet(prov))
			{
				auto allow_new_unpins=true;
				total_unpins++;
				if(do_unpin_opt != -1 && total_unpins > do_unpin_opt) 
				{
					allow_new_unpins=false;
					
					// don't abort early as we need to fully unpin the IBT once we've started.
					//cout<<"Aborting unpin process mid switch table."<<endl;
					// return;
				}

				// if we are out of unpins, and we haven't unpinned this instruction yet, skip unpinning it.
				if(!allow_new_unpins && already_unpinned.find(ibt)==already_unpinned.end())
				{
					if(getenv("UNPIN_VERBOSE"))
						cout<<"Eliding switch unpin for entry["<<dec<<i<<"] ibt="<<hex<<table_entry<<", scoop_off="<<scoop_off<<endl;

					// do nothing
				}
				else
				{
					if(getenv("UNPIN_VERBOSE"))
						cout<<"Unpinning switch entry ["<<dec<<i<<"] for ibt="<<hex<<table_entry<<", scoop_off="<<scoop_off<<endl;
					already_unpinned.insert(ibt);

					Relocation_t* nr=new Relocation_t();
					assert(nr);
					nr->SetType("data_to_insn_ptr");
					nr->SetOffset(scoop_off);
					nr->SetWRT(ibt);
					// add reloc to IR.
					firp->GetRelocations().insert(nr);
					scoop->GetRelocations().insert(nr);

					// remove rodata reference for hell nodes.
					targets[table_entry]=newprov;
					switch_targs.insert(ibt);
				}
			}
		}
		scoop_off+=ptrsize;
		
		
	}

	type3_unpins+=switch_targs.size();
	type3_pins+=jmptables[insn].size();

}

void unpin_switches(FileIR_t *firp, int do_unpin_opt)
{
	// re-init stats for this file.
	type3_unpins=0;
	type3_pins=0;

	// for each instruction 
        for(
                set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
                it!=firp->GetInstructions().end();
                ++it
	   )

        {
		// check for an insn.
		Instruction_t* insn=*it;
		assert(insn);

		// if we didn't find a jmptable for this insn, try again.
		if(jmptables.find(insn) == jmptables.end())
			continue;

		// sanity check we have a good switch 
		if(insn->GetIBTargets()==NULL) continue;

		// sanity check we have a good switch 
		if(insn->GetIBTargets()->GetAnalysisStatus()!=ICFS_Analysis_Complete) continue;

		// find the scoop, try next if we fail.
		DataScoop_t* scoop=find_scoop(firp,jmptables[insn].GetTableStart());
		if(!scoop) continue;

		if(jmptables[insn].GetSwitchType().areOnlyTheseSet(ibt_provenance_t::ibtp_switchtable_type3))
		{
			unpin_type3_switchtable(firp,insn,scoop, do_unpin_opt);
		}
        }
	cout<<"# ATTRIBUTE fill_in_indtargs::switch_type3_pins="<<dec<<type3_pins<<endl;
	cout<<"# ATTRIBUTE fill_in_indtargs::switch_type3_unpins="<<dec<<type3_unpins<<endl;
}

void print_unpins(FileIR_t *firp)
{
	// don't print if not asked for this type of verbose 
	if(getenv("UNPIN_VERBOSE") == NULL)
		return;

	for(
		DataScoopSet_t::iterator it=firp->GetDataScoops().begin();
		it!=firp->GetDataScoops().end();
		++it
	   )
	{
		DataScoop_t* scoop=*it;
		assert(scoop);
		for(
			RelocationSet_t::iterator rit=scoop->GetRelocations().begin();
			rit!=scoop->GetRelocations().end();
			++rit
		   )
		{
			Relocation_t* reloc=*rit;
			assert(reloc);
			cout<<"Found relocation in "<<scoop->GetName()<<" of type "<<reloc->GetType()<<" at offset "<<hex<<reloc->GetOffset()<<endl;
		}
	}
}


void unpin_well_analyzed_ibts(FileIR_t *firp, int64_t do_unpin_opt)
{
	unpin_elf_tables(firp, do_unpin_opt);
	unpin_switches(firp, do_unpin_opt);
	print_unpins(firp);
}



/*
 * fill_in_indtargs - main driver routine for 
 */
void fill_in_indtargs(FileIR_t* firp, exeio* elfiop, std::list<virtual_offset_t> forced_pins, int64_t do_unpin_opt)
{
	set<virtual_offset_t> thunk_bases;
	find_all_module_starts(firp,thunk_bases);

	// reset global vars
	bounds.clear();
	ranges.clear();
	targets.clear();
	jmptables.clear();
	already_unpinned.clear();
	lookupInstruction_init(firp);

	calc_preds(firp);

        int secnum = elfiop->sections.size();
	int secndx=0;

	/* look through each section and record bounds */
        for (secndx=0; secndx<secnum; secndx++)
		get_executable_bounds(firp, elfiop->sections[secndx]);

	/* import info from stars */
	read_stars_xref_file(firp);

	/* look through each section and look for target possibilities */
        for (secndx=0; secndx<secnum; secndx++)
		infer_targets(firp, elfiop->sections[secndx]);
	
	/* should move to separate function */
	std::list<virtual_offset_t>::iterator forced_iterator = forced_pins.begin();
	for (; forced_iterator != forced_pins.end(); forced_iterator++)
	{
		possible_target(*forced_iterator, 0, ibt_provenance_t::ibtp_user);
	}

	/* look through the instructions in the program for targets */
	get_instruction_targets(firp, elfiop, thunk_bases);

	/* mark the entry point as a target */
	possible_target(elfiop->get_entry(),0,ibt_provenance_t::ibtp_entrypoint); 

	/* Read the exception handler frame so that those indirect branches are accounted for */
	/* then now process the ranges and mark IBTs as necessarthat have exception handling */
        read_ehframe(firp, elfiop);
	process_ranges(firp);
	
	/* now, find the .GOT addr and process any pc-rel things for x86-32 ibts. */
	check_for_thunks(firp, thunk_bases);

	/* now deal with dynsym pins */
	process_dynsym(firp);

	/* mark any instructions after a call instruction */
	mark_return_points(firp);

	/* set the IR to have some instructions marked as IB targets, and deal with the ICFS */
	mark_targets(firp);
	

	cout<<"========================================="<<endl;
	cout<<"# ATTRIBUTE fill_in_indtargs::total_indirect_targets="<<std::dec<<targets.size()<<endl;
	print_targets();
	cout<<"========================================="<<endl;

	cout<<"========================================="<<endl;
	cout<<"# ATTRIBUTE oneline_indirect_targets=";
	print_targets_oneline();
	cout<<"========================================="<<endl;

	// try to setup an ICFS for every IB.
	setup_icfs(firp, elfiop);

	// do unpinning of well analyzed ibts.
	if(do_unpin_opt!=(int64_t)-1) 
		unpin_well_analyzed_ibts(firp, do_unpin_opt);

}


int main(int argc, char* argv[])
{
	auto argc_iter = (int)2;
	auto split_eh_frame_opt=true;
	auto do_unpin_opt=numeric_limits<int64_t>::max() ;
	auto forced_pins=std::list<virtual_offset_t> ();

	if(argc<2)
	{
		cerr<<"Usage: fill_in_indtargs <id> [--[no-]split-eh-frame] [--[no-]unpin] [addr,...]"<<endl;
		exit(-1);
	}

	// parse dash-style options.
	while(argc_iter < argc && argv[argc_iter][0]=='-')
	{
		if(string(argv[argc_iter])=="--no-unpin")
		{
			do_unpin_opt=-1;
			argc_iter++;
		}
		else if(string(argv[argc_iter])=="--unpin")
		{
			do_unpin_opt = numeric_limits<decltype(do_unpin_opt)>::max() ;
			argc_iter++;
		}
		else if(string(argv[argc_iter])=="--max-unpin" || string(argv[argc_iter])=="--max-unpins")
		{
			argc_iter++;
			auto arg_as_str=argv[argc_iter];
			argc_iter++;

			try { 
				do_unpin_opt = stoul(arg_as_str,NULL,0);
			}
			catch (invalid_argument ia)
			{
				cerr<<"In --max-unpin, cannot convert "<<arg_as_str<<" to unsigned"<<endl;
				exit(1);
			}
			
		}
		else if(string(argv[argc_iter])=="--no-split-eh-frame")
		{
			split_eh_frame_opt=false;
			argc_iter++;
		}
		else if(string(argv[argc_iter])=="--split-eh-frame")
		{
			split_eh_frame_opt=true;
			argc_iter++;
		}
		else
		{
			cerr<<"Unknown option: "<<argv[argc_iter]<<endl;
			exit(2);
		}
	}
	// parse addr argumnets 
	for (; argc_iter < argc; argc_iter++)
	{
		char *end_ptr;
		virtual_offset_t offset = strtol(argv[argc_iter], &end_ptr, 0);
		if (*end_ptr == '\0')
		{
			cout << "force pinning: 0x" << std::hex << offset << endl;
			forced_pins.push_back(offset);
		}
	}

	VariantID_t *pidp=NULL;
	FileIR_t * firp=NULL;

	try 
	{
		/* setup the interface to the sql server */
		pqxxDB_t pqxx_interface;
		BaseObj_t::SetInterface(&pqxx_interface);

		pidp=new VariantID_t(atoi(argv[1]));


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


			// read the executeable file
			int elfoid=firp->GetFile()->GetELFOID();
		        pqxx::largeobject lo(elfoid);
        		lo.to_file(pqxx_interface.GetTransaction(),"readeh_tmp_file.exe");
        		EXEIO::exeio*    elfiop=new EXEIO::exeio;
        		elfiop->load(string("readeh_tmp_file.exe"));

			// find all indirect branch targets
			fill_in_indtargs(firp, elfiop, forced_pins, do_unpin_opt);
			if(split_eh_frame_opt)
				split_eh_frame(firp);
			
			// write the DB back and commit our changes 
			firp->WriteToDB();

			delete firp;
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
	return 0;
}
