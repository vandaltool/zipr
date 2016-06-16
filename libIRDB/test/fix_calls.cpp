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
#include <libIRDB-cfg.hpp>
#include <utils.hpp>
#include <iostream>
#include <stdlib.h>
#include "beaengine/BeaEngine.h"
#include <assert.h>
#include <string.h>
#include <elf.h>
#include <set>
#include <exeio.h>

#include "fill_in_indtargs.hpp"


using namespace libIRDB;
using namespace std;
using namespace EXEIO;



class Range_t
{
        public:
                Range_t(virtual_offset_t p_s, virtual_offset_t p_e) : m_start(p_s), m_end(p_e) { }
                Range_t() : m_start(0), m_end(0) { }

                virtual virtual_offset_t GetStart() const { return m_start; }
                virtual virtual_offset_t GetEnd() const { return m_end; }
                virtual void SetStart(virtual_offset_t s) { m_start=s; }
                virtual void SetEnd(virtual_offset_t e) { m_end=e; }

        protected:

                virtual_offset_t m_start, m_end;
};

struct Range_tCompare
{
        bool operator() (const Range_t &first, const Range_t &second) const
        {
                return first.GetEnd() < second.GetStart();
        }
};

typedef std::set<Range_t, Range_tCompare> RangeSet_t;



RangeSet_t eh_frame_ranges;
long long no_target_insn=0;
long long no_fallthrough_insn=0;
long long target_not_in_function=0;
long long call_to_not_entry=0;
long long thunk_check=0;
long long found_pattern=0;
long long in_ehframe=0;
long long no_fix_for_ib=0;

pqxxDB_t pqxx_interface;

bool opt_fix_icalls = true;

void fix_other_pcrel(FileIR_t* firp, Instruction_t *insn, UIntPtr offset);

/* Read the exception handler frame so that those indirect branches are accounted for */
void read_ehframe(FileIR_t* firp, EXEIO::exeio* );



bool check_entry(bool &found, ControlFlowGraph_t* cfg)
{

	BasicBlock_t *entry=cfg->GetEntry();
	found=false;

	for(
		std::vector<Instruction_t*>::const_iterator it=entry->GetInstructions().begin();
		it!=entry->GetInstructions().end();
		++it
	   )
	{
		DISASM disasm;
		Instruction_t* insn=*it;
		insn->Disassemble(disasm);
		if(Instruction_t::SetsStackPointer(&disasm)) {
			return false;
		} else {
			if(getenv("VERBOSE_FIX_CALLS"))
			{
				virtual_offset_t addr = 0;
				if (insn->GetAddress())
					addr = insn->GetAddress()->GetVirtualOffset();
				cout<<"check_entry: does not set stack pointer?"<< " address="
				    <<hex<<addr<<": "<<insn->getDisassembly()<<endl;
			}
		}

		if(strstr(disasm.CompleteInstr, "[esp]"))
		{
			found=true;
			if(getenv("VERBOSE_FIX_CALLS"))
			{
				virtual_offset_t addr = 0;
				if (insn->GetAddress())
					addr = insn->GetAddress()->GetVirtualOffset();
				cout<<"Needs fix (check_entry): [esp]"<< " address="
				    <<hex<<addr<<": "<<insn->getDisassembly()<<endl;
			}
			return true;
		}
		
	}


	return false;
}


bool call_needs_fix(Instruction_t* insn)
{

	for(set<Relocation_t*>::iterator it=insn->GetRelocations().begin();
		it!=insn->GetRelocations().end();
		++it
	   )
	{
		Relocation_t* reloc=*it;
		if(string("safefr") == reloc->GetType())
			return false;
	}

	Instruction_t *target=insn->GetTarget();
	Instruction_t *fallthru=insn->GetFallthrough();
	DISASM disasm;

	string pattern;

// this used to work because fill_in_indirects would mark IBTs 
// while reading the ehframe, which perfectly corresponds to when
// we need to fix calls due to eh_frame.  However, now STARS also marks
// return points as IBTs, so we need to re-parse the ehframe and use that instead.

	/* no fallthrough instruction, something is odd here */
	if(!fallthru)
	{
		if(getenv("VERBOSE_FIX_CALLS"))
		{
			virtual_offset_t addr = 0;
			if (insn->GetAddress())
				addr = insn->GetAddress()->GetVirtualOffset();
			cout<<"Needs fix: No fallthrough"<< " address="
			    <<hex<<addr<<": "<<insn->getDisassembly()<<endl;
		}
		no_fallthrough_insn++;
		return true;
	}

#if 0
	if(fallthru->GetIndirectBranchTargetAddress()!=NULL)
		return true;
#else
	virtual_offset_t addr=fallthru->GetAddress()->GetVirtualOffset();
	RangeSet_t::iterator rangeiter=eh_frame_ranges.find(Range_t(addr,addr));
	if(rangeiter != eh_frame_ranges.end())	// found an eh_frame addr entry for this call
	{
		in_ehframe++;
		return true;
	}
#endif

	if (!opt_fix_icalls && insn->GetIBTargets() && insn->GetIBTargets()->size() > 0) 
	{
		/* do not fix indirect calls */
		no_fix_for_ib++;
		return false;
	}

	/* if the target isn't in the IR */
	if(!target)
	{
		/* call 0's aren't to real locations */
		insn->Disassemble(disasm);
		if(strcmp(disasm.CompleteInstr, "call 0x00000000")==0)
		{
			return false;
		}
		no_target_insn++;

		if(getenv("VERBOSE_FIX_CALLS"))
		{
			virtual_offset_t addr = 0;
			if (insn->GetAddress())
				addr = insn->GetAddress()->GetVirtualOffset();
			cout<<"Needs fix: No target instruction"<< " address="
			    <<hex<<addr<<": "<<insn->getDisassembly()<<endl;
		}
		/* then we need to fix it */
		return true;
	}


	/* if the location after the call is marked as an IBT, then 
	 * this location might be used for walking the stack 
  	 */


	Function_t* func=target->GetFunction();

	/* if there's no function for this instruction */
	if(!func)
	{
		if(getenv("VERBOSE_FIX_CALLS"))
		{
			virtual_offset_t addr = 0;
			if (insn->GetAddress())
				addr = insn->GetAddress()->GetVirtualOffset();
			cout<<"Needs fix: Target not in a function"<< " address="
			    <<hex<<addr<<": "<<insn->getDisassembly()<<endl;
		}
		target_not_in_function++;
		/* we need to fix it */
		return true;
	}


	/* build a cfg for this function */
	ControlFlowGraph_t* cfg=new ControlFlowGraph_t(func);

	assert(cfg->GetEntry());

	/* if the call instruction isn't to a function entry point */
	if(cfg->GetEntry()->GetInstructions()[0]!=target)
	{
		call_to_not_entry++;
		/* then we need to fix it */
		return true;
	}


	/* check the entry block for thunks, etc. */
	bool found;
	bool ret=check_entry(found,cfg);
	delete cfg;
	if(found)
	{
		if(ret)
		{
			if(getenv("VERBOSE_FIX_CALLS"))
			{
				virtual_offset_t addr = 0;
				if (insn->GetAddress())
					addr = insn->GetAddress()->GetVirtualOffset();
				cout<<"Needs fix: (via check_entry) Thunk detected"<< " address="
				    <<hex<<addr<<": "<<insn->getDisassembly()<<endl;
			}
			thunk_check++;
		}

		return ret;
	}

	/* now, search the function for stack references  */


	/* determine what the stack ref. would look like */
	if(func->GetUseFramePointer())
	{
		pattern="[ebp+0x04]";
	}
	else
	{
		pattern="[esp+"+to_string(func->GetStackFrameSize())+"]";
	}


	/* check each instruction */
	for(
		std::set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
		it!=func->GetInstructions().end();
		++it
	   )
	{
		Instruction_t* itrinsn=*it;
		/* if the disassembly contains the string mentioned */
		DISASM disasm;
		itrinsn->Disassemble(disasm);
		if(strstr(disasm.CompleteInstr, pattern.c_str())!=NULL) 
		{
			found_pattern++;
			if(getenv("VERBOSE_FIX_CALLS"))
			{
				virtual_offset_t addr = 0;
				if (insn->GetAddress())
					addr = insn->GetAddress()->GetVirtualOffset();
				cout<<"Needs fix: Found pattern"<< " address="
				    <<hex<<addr<<": "<<insn->getDisassembly()<<endl;
			}
			/* then we need to fix this callsite */ 
			return true;
		}
	}

	/* otherwise, we think it's safe */
	return false;

}

/*
 * - adjust_esp_offset - take newbits, and determine if it has an esp+K type offset in a memory address.
 * if so, adjust k by offset, and return the new string.
 */
string adjust_esp_offset(string newbits, int offset)
{

        /*
         * call has opcodes of:
         *              E8 cw   call near, relative, displacement relative to next instruction
         *              E8 cd   call near, relative, displacement relative to next instruction
         *              FF /2   call near, absolute indirect, address given in r/m16
         *              FF /2   call near, absolute indirect, address given in r/m32
         *              9a cd   call far, absolute, address given in operand
         *              9a cp   call far, absolute, address given in operand
         *              FF /3   call far, absolute indirect, address given in m16:16
         *              FF /3   call far, absolute indirect, address given in m16:32
         *
         *              FF /4   jmp near, absolute indirect, address given in r/m16
         *              FF /4   jmp near, absolute indirect, address given in r/m32
         *              FF /5   jmp near, absolute indirect, address given in r/m16
         *              FF /5   jmp near, absolute indirect, address given in r/m32
         *
         *              /digit indicates that the Reg/Opcode field (bits 3-5) of the ModR/M byte (opcode[1])
         *              contains that value as an opcode instead of a register operand indicator.  The instruction only
         *              uses the R/M field (bits 0-2) as an operand.
         *
         *              cb,cw,cd,cp indicates a 1,2,4,6-byte following the opcode is used to specify a
         *                      code offset and possibly a new code segment
         *
         *      I believe we only care about these this version:
         *              FF /3   call far, absolute indirect, address given in m16:32
         *      as we're looking for an address on the stack.
         *
         *      The ModR/M byte must be Mod=10, Reg/Op=010, R/M=100 aka 0x94 or possibly
         *                              Mod=01, Reg/Op=100, R/M=100 aka 0x64 or possibly
         *                              Mod=10, Reg/Op=100, R/M=100 aka 0xa4 or possibly
         *                              Mod=01, Reg/Op=010, R/M=100 aka 0x54 or possibly
         *      R/M=100 indicates to read the SIB (see below)
         *      Mod=10 indicates an 32-bit displacement after the SIB byte
         *      Mod=01 indicates an  8-bit displacement after the SIB byte
         *      Reg/Op=010 indicates an opcode that has a /3 after it
         *      Reg/Op=100 indicates an opcode that has a /5 after it
         *
         *      The SIB byte (opcode[2]) needs to be scale=00, index=100, base=100, or 0x24
         *              indicating that the addresing mode is 0*%esp+%esp.
         *
         *      I believe that the SIB byte could also be 0x64, 0xA4, or 0xE4, but that
         *      these are very rarely used as they are redundant.
         *
         *
         *
         *
         */
        /* non-negative offsets please */
        assert(offset>=0);

        int sib_byte=(unsigned char)newbits[2];
        int sib_base=(unsigned char)sib_byte&0x7;
        int sib_ss=(sib_byte>>3)&0x7;
        int sib_index=(sib_byte>>3)&0x7;


        /* 32-bit offset */
        if ( (unsigned char)newbits[0] == 0xff &&                                	/* ff */
             ((unsigned char)newbits[1] == 0x94 || (unsigned char)newbits[1] == 0x64 )    && 		/* /2  or /4*/
             sib_base == 0x4 )                                          /* base==esp */
        {
		// reconstruct the old 32-bit value
		int oldval=((unsigned char)newbits[6]<<24)+((unsigned char)newbits[5]<<16)+((unsigned char)newbits[4]<<8)+((unsigned char)newbits[3]);

		// add the offset 
		int newval=oldval+offset;

		// break it back apart to store in the string.
		newbits[3]=(char)(newval>>0)&0xff;
		newbits[4]=(char)(newval>>8)&0xff;
		newbits[5]=(char)(newval>>16)&0xff;
		newbits[6]=(char)(newval>>24)&0xff;
        }

        /* 8-bit offset */
        else if ( (unsigned char)newbits[0] == 0xff &&                           	/* ff */
             ((unsigned char)newbits[1] == 0x54 || (unsigned char)newbits[1]==0xa4) &&    		/* /3 or /5 */
             sib_base == 0x4 )                                          /* base==esp */
        {
                /* We need to add 4 to the offset, but this may overflow an 8-bit quantity
                 * (note:  there's no 16-bit offset for this insn.  Only 8-bit or 32-bit offsets exist for this instr)
                 * if the const offset is over (0x7f-offset), adding offset will overflow it to be negative instead
                 * of positive
                 */
                if((unsigned char)newbits[3]>=(0x7f-offset))
                {
                        /* Careful, adding 4 to this will cause an overflow.
                         * We need to convert it to a 32-bit displacement
                         */
                        /* first, change addressing mode from 8-bit to 32-bit. */
                        newbits[1]&=0x3f;         /* remove upper 2 bits */
                        newbits[1]|=0x80;         /* make them 10 to indicate a 32-bit offset */

			// sign-extend to 32-bit int
			int oldval=(char)newbits[3];

			// add the offset 
			int newval=oldval+offset;
	
			// break it back apart to store in the string.
			assert(newbits.length() == 4);
			newbits[3]=(char)(newval>>0)&0xff;
			// 3 most significant bytes extend the instruction
			newbits+=(char)(newval>>8)&0xff;
			newbits+=(char)(newval>>16)&0xff;
			newbits+=(char)(newval>>24)&0xff;

                }
                else

                        newbits[3] += (char)offset;
        }
	return newbits;
}

	

/* 
 * convert_to_jump - assume newbits is a call insn, convert newbits to a jump, and return it.
 * Also: if newbits is a call [esp+k], add "offset" to k.
 */ 
static void convert_to_jump(Instruction_t* insn, int offset)
{
	string newbits=insn->GetDataBits();
	DISASM d;
	insn->Disassemble(d);
	/* this case is odd, handle it specially (and more easily to understand) */
	if(strcmp(d.CompleteInstr, "call qword [rsp]")==0)
	{
		char buf[100];
		sprintf(buf,"jmp qword [rsp+%d]", offset);
		insn->Assemble(buf);
		return;
	}


	/* skip over any prefixes */
	int op_index=d.Prefix.Number;

	// on the opcode.  assume no prefix here 	
	switch((unsigned char)newbits[op_index])
	{
		// opcodes: ff /2 and ff /3
		case 0xff:	
		{
			// opcode: ff /2
			// call r/m32, call near, absolute indirect, address given in r/m32
			if(((((unsigned char)newbits[op_index+1])&0x38)>>3) == 2)
			{
				newbits=adjust_esp_offset(newbits,offset);
				// convert to jmp r/m32
				// opcode: FF /4
				newbits[op_index+1]&=0xC7;	// remove old bits
				newbits[op_index+1]|=(0x4<<3);	// set r/m field to 4
			}
			// opcode: ff /3
			// call m16:32, call far, absolute indirect, address given in m16:32	
			else if(((((unsigned char)newbits[op_index+1])&0x38)>>3) == 3)
			{
				// convert to jmp m16:32
				// opcode: FF /5
				newbits[op_index+1]&=0xC7;	// remove old bits
				newbits[op_index+1]|=(0x5<<3);	// set r/m field to 5
			}
			else
				assert(0);
			break;
		}
		// opcode: 9A cp
		// call ptr16:32, call far, absolute, address given in operand
		case 0x9A:	
		{
			// convert to jmp ptr16:32
			// opcode: EA cp
			newbits[op_index+0]=0xEA;
			break;
		}

		// opcode: E8 cd
		// call rel32, call near, relative, displacement relative to next insn.
		case 0xE8:
		{
			// convert to jmp rel32
			// opcode: E9 cd
			newbits[op_index+0]=0xE9;
			break;
		}

		// not a call
		default:
			assert(0);
	}

	/* set the instruction's bits */
	insn->SetDataBits(newbits);
	return;
}


/* 
 * fix_call - convert call to push/jump.
 */
void fix_call(Instruction_t* insn, FileIR_t *firp, bool can_unpin)
{
	/* record the possibly new indirect branch target if this call gets fixed */
	Instruction_t* newindirtarg=insn->GetFallthrough();
	bool has_rex=false;

	/* disassemble */
        DISASM disasm;

        /* Disassemble the instruction */
        int instr_len = insn->Disassemble(disasm);


	/* if this instruction is an inserted call instruction than we don't need to 
	 * convert it for correctness' sake.
	 */
	if(insn->GetAddress()->GetVirtualOffset()==0)
		return;

	/* if the first byte isn't a call opcode, there's some odd prefixing and we aren't handling it.
	 * this comes up most frequently in a call gs:0x10 instruction where an override prefix specifes the gs: part.
	 */
	if((insn->GetDataBits()[0]&0x40)==0x40)
	{
		// has rex!
		has_rex=true;
	}
	else if( (insn->GetDataBits()[0]!=(char)0xff) && 
		 (insn->GetDataBits()[0]!=(char)0xe8) && 
		 (insn->GetDataBits()[0]!=(char)0x9a) )
	{
		cout<<"Found odd prefixing.\n  Not handling **********************************************"<<endl;
		assert(0);
		return;
	}

	if(getenv("VERBOSE_FIX_CALLS"))
	{
		cout<<"Doing a fix_call on "<<std::hex<<insn->GetAddress()->GetVirtualOffset()<< " which is "<<disasm.CompleteInstr<<endl;
	}


	virtual_offset_t next_addr=insn->GetAddress()->GetVirtualOffset() + insn->GetDataBits().length();

	/* create a new instruction and a new addresss for it that do not correspond to any original program address */
	Instruction_t *callinsn=new Instruction_t();
	AddressID_t *calladdr=new AddressID_t;
       	calladdr->SetFileID(insn->GetAddress()->GetFileID());

	/* set the fields in the new instruction */
	callinsn->SetAddress(calladdr);
	callinsn->SetTarget(insn->GetTarget());
	callinsn->SetFallthrough(NULL);
	callinsn->SetFunction(insn->GetFunction());
	callinsn->SetComment(insn->GetComment()+" Jump part");

	/* handle ib targets */
	callinsn->SetIBTargets(insn->GetIBTargets());
	insn->SetIBTargets(NULL);

	// We need the control transfer instruction to be from the orig program because 
	// if for some reason it's fallthrough/target isn't in the DB, we need to correctly 
	// emit fallthrough/target rules
	callinsn->SetOriginalAddressID(insn->GetOriginalAddressID());
	insn->SetOriginalAddressID(BaseObj_t::NOT_IN_DATABASE);

	/* set the new instruction's data bits to be a jmp instead of a call */
	string newbits="";

	/* add 4 (8) if it's an esp(rsp) indirect branch for x86-32 (-64) */ 
	callinsn->SetDataBits(insn->GetDataBits());
	convert_to_jump(callinsn,firp->GetArchitectureBitWidth()/8);		

	/* the jump instruction should NOT be indirectly reachable.  We should
	 * land at the push
	 */
	fix_other_pcrel(firp, callinsn, insn->GetAddress()->GetVirtualOffset());
	callinsn->SetIndirectBranchTargetAddress(NULL);

	/* add the new insn and new address into the list of valid calls and addresses */
	firp->GetAddresses().insert(calladdr);
	firp->GetInstructions().insert(callinsn);


	/* Convert the old call instruction into a push return_address instruction */
	insn->SetFallthrough(callinsn);
	insn->SetTarget(NULL);
	newbits=string("");
	newbits.resize(5);
	newbits[0]=0x68;	/* assemble an instruction push next_addr */
	newbits[1]=(next_addr>>0) & 0xff;
	newbits[2]=(next_addr>>8) & 0xff;
	newbits[3]=(next_addr>>16) & 0xff;
	newbits[4]=(next_addr>>24) & 0xff;
	insn->SetDataBits(newbits);
	insn->SetComment(insn->GetComment()+" Push part");

	/* create a relocation for this instruction */
	Relocation_t* reloc=new Relocation_t;
	if(firp->GetArchitectureBitWidth()==32)
	{
		reloc->SetOffset(1);
		reloc->SetType("32-bit");
	}
	else
	{
		assert(firp->GetArchitectureBitWidth()==64);
		reloc->SetOffset(0);
		reloc->SetType("push64");
	}


	insn->GetRelocations().insert(reloc);
	firp->GetRelocations().insert(reloc);

	/* If the fallthrough is not marked as indirectly branchable-to, then mark it so */
	if(newindirtarg && !newindirtarg->GetIndirectBranchTargetAddress())
	{
		/* create a new address for the IBTA */
		AddressID_t* newaddr = new AddressID_t;
		assert(newaddr);
		newaddr->SetFileID(newindirtarg->GetAddress()->GetFileID());
		newaddr->SetVirtualOffset(newindirtarg->GetAddress()->GetVirtualOffset());

		/* set the instruction and include this address in the list of addrs */
		newindirtarg->SetIndirectBranchTargetAddress(newaddr);
		firp->GetAddresses().insert(newaddr);
	
		// if we're marking this as an IBTA, determine whether we can unpin it or not 
		if(can_unpin)
		{
			if(getenv("VERBOSE_FIX_CALLS"))
			{
				cout<<"Setting unpin for type="<< reloc->GetType()<< " address="
				    <<hex<<insn->GetBaseID()<<":"<<insn->getDisassembly()<<endl;
			}
			reloc->SetWRT(newindirtarg);
		}
	}


	// mark in the IR what the fallthrough of this insn is.
	Relocation_t* fix_call_reloc=new Relocation_t(); 
	fix_call_reloc->SetOffset(0);
	fix_call_reloc->SetType("fix_call_fallthrough");
	fix_call_reloc->SetWRT(newindirtarg);
	callinsn->GetRelocations().insert(fix_call_reloc);
	firp->GetRelocations().insert(fix_call_reloc);

}


//
// return true if insn is a call
//
bool is_call(Instruction_t* insn)
{

        DISASM disasm;
        memset(&disasm, 0, sizeof(DISASM));
#if 0
        disasm.Options = NasmSyntax + PrefixedNumeral;
        disasm.Archi = 32;
        disasm.EIP = (UIntPtr)insn->GetDataBits().c_str();
        disasm.VirtualAddr = insn->GetAddress()->GetVirtualOffset();
#endif

        /* Disassemble the instruction */
        int instr_len = insn->Disassemble(disasm);
	

	return (disasm.Instruction.BranchType==CallType);
}

static File_t* find_file(FileIR_t* firp, db_id_t fileid)
{
#if 0
        set<File_t*> &files=firp->GetFiles();

        for(
                set<File_t*>::iterator it=files.begin();
                it!=files.end();
                ++it
           )
        {
                File_t* thefile=*it;
                if(thefile->GetBaseID()==fileid)
                        return thefile;
        }
        return NULL;
#endif
        assert(firp->GetFile()->GetBaseID()==fileid);
        return firp->GetFile();

}


template <class T> struct insn_less : binary_function <T,T,bool> {
  bool operator() (const T& x, const T& y) const {
        return  x->GetBaseID()  <   y->GetBaseID()  ;}
};


//
// fix_all_calls - convert calls to push/jump pairs in the IR.  if fix_all is true, all calls are converted, 
// else we attempt to detect the calls it is safe to convert.
//
void fix_all_calls(FileIR_t* firp, bool print_stats, bool fix_all)
{

        set<Instruction_t*,insn_less<Instruction_t*> > sorted_insns;

        for(
                set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
                it!=firp->GetInstructions().end();
                ++it
           )
        {
                Instruction_t* insn=*it;
                sorted_insns.insert(insn);
        }

	long long fixed_calls=0, not_fixed_calls=0, not_calls=0;

	for(
		set<Instruction_t*,insn_less<Instruction_t*> >::const_iterator it=sorted_insns.begin();
		it!=sorted_insns.end(); 
		++it
	   )
	{

		Instruction_t* insn=*it;
		if(getenv("STOP_FIX_CALLS_AT") && fixed_calls>=atoi(getenv("STOP_FIX_CALLS_AT")))
			break;

		if(is_call(insn)) 
		{
			if( call_needs_fix(insn) )	// fixing is necessary + unpinning not possible.
			{
				fixed_calls++;
				fix_call(insn, firp, false);
			}
			// we've been asked to fix all calls for funsies/cfi
			// (and a bit about debugging fix-calls that's not important for anyone but jdh.
			else if ( fix_all || (getenv("FIX_CALL_LIMIT") && not_fixed_calls>=atoi(getenv("FIX_CALL_LIMIT"))))
			{
				// if we make it here, we know that it was not 100% necessary to fix the call
				// but we've been asked to anyhow.	
				fixed_calls++;
				fix_call(insn, firp, true /* true here indicates that the call can have an unpin reloc -- anh to add option in 3 minutes */);
			}
			else
			{
				if(getenv("VERBOSE_FIX_CALLS"))
					cout<<"no fix needed for "<<insn->GetAddress()->GetVirtualOffset()<<":"<<insn->getDisassembly()<<endl;
				not_fixed_calls++;
			}
		}
		
		else
		{
			if(getenv("VERBOSE_FIX_CALLS"))
				cout<<"Not a call "<<insn->GetAddress()->GetVirtualOffset()<<":"<<insn->getDisassembly()<<endl;
			not_calls++;
		}
	}


	if(print_stats)
	{
		cout << "# ATTRIBUTE fixed_calls="<<std::dec<<fixed_calls<<endl;
		cout << "# ATTRIBUTE no_fix_needed_calls="<<std::dec<<not_fixed_calls<<endl;
		cout << "# ATTRIBUTE other_instructions="<<std::dec<<not_calls<<endl;
		cout << "# ATTRIBUTE fixed_ratio="<<std::dec<<(fixed_calls/((float)(not_fixed_calls+fixed_calls)))<<endl;
		cout << "# ATTRIBUTE remaining_ratio="<<std::dec<<(not_fixed_calls/((float)(not_fixed_calls+fixed_calls+not_calls)))<<endl;
		cout << "# ATTRIBUTE no_target_insn="<<std::dec<< no_target_insn << endl;
		cout << "# ATTRIBUTE no_fallthrough_insn="<<std::dec<< no_fallthrough_insn << endl;
		cout << "# ATTRIBUTE target_not_in_function="<<std::dec<< target_not_in_function << endl;
		cout << "# ATTRIBUTE call_to_not_entry="<<std::dec<< call_to_not_entry << endl;
		cout << "# ATTRIBUTE thunk_check="<<std::dec<< thunk_check << endl;
		cout << "# ATTRIBUTE found_pattern="<<std::dec<< found_pattern << endl;
		cout << "# ATTRIBUTE in_ehframe="<<std::dec<< in_ehframe << endl;
		cout << "# ATTRIBUTE no_fix_for_ib="<<std::dec<< no_fix_for_ib << endl;
		no_target_insn=0;
		no_fallthrough_insn=0;
		target_not_in_function=0;
		call_to_not_entry=0;
		thunk_check=0;
		found_pattern=0;
		in_ehframe=0;
		no_fix_for_ib=0;
	}
}

bool arg_has_relative(const ARGTYPE &arg)
{
	/* if it's relative memory, watch out! */
	if(arg.ArgType&MEMORY_TYPE)
		if(arg.ArgType&RELATIVE_)
			return true;
	
	return false;
}


//
//  fix_other_pcrel - add relocations to other instructions that have pcrel bits
//
void fix_other_pcrel(FileIR_t* firp, Instruction_t *insn, UIntPtr virt_offset)
{
	DISASM disasm;
	insn->Disassemble(disasm);
	int is_rel= arg_has_relative(disasm.Argument1) || arg_has_relative(disasm.Argument2) || arg_has_relative(disasm.Argument3);

	/* if this has already been fixed, we can skip it */
	if(virt_offset==0 || virt_offset==-1)
		return;

	if(is_rel)
	{
		ARGTYPE* the_arg=NULL;
		if(arg_has_relative(disasm.Argument1))
			the_arg=&disasm.Argument1;
		if(arg_has_relative(disasm.Argument2))
			the_arg=&disasm.Argument2;
		if(arg_has_relative(disasm.Argument3))
			the_arg=&disasm.Argument3;

		assert(the_arg);

		int offset=the_arg->Memory.DisplacementAddr-disasm.EIP;
		assert(offset>=0 && offset <=15);
		int size=the_arg->Memory.DisplacementSize;
		assert(size==1 || size==2 || size==4 || size==8);

		if(getenv("VERBOSE_FIX_CALLS"))
		{
			cout<<"Found insn with pcrel memory operand: "<<disasm.CompleteInstr
		    	    <<" Displacement="<<std::hex<<the_arg->Memory.Displacement<<std::dec
		    	    <<" size="<<the_arg->Memory.DisplacementSize<<" Offset="<<offset;
		}

		/* convert [rip_pc+displacement] addresssing mode into [rip_0+displacement] where rip_pc is the actual PC of the insn, 
		 * and rip_0 is means that the PC=0. AKA, we are relocating this instruction to PC=0. Later we add a relocation to undo this transform at runtime 
		 * when we know the actual address.
		 */

		/* get the data */
		string data=insn->GetDataBits();
		char cstr[20]; 
		memcpy(cstr,data.c_str(), data.length());
		void *offsetptr=&cstr[offset];

		UIntPtr disp=the_arg->Memory.Displacement;
		UIntPtr oldpc=virt_offset;
		UIntPtr newdisp=disp+oldpc;

		assert(offset+size<=data.length());
		
		switch(size)
		{
			case 4:
				assert( (UIntPtr)(int)newdisp == (UIntPtr)newdisp);
				*(int*)offsetptr=newdisp;
				break;
			case 1:
			case 2:
			case 8:
			default:
				assert(("Cannot handle offset of given size", 0));
		}

		/* put the data back into the insn */
		data.replace(0, data.length(), cstr, data.length());
		insn->SetDataBits(data);

		// going to end up in the SPRI file anyhow after changing the data bits 
		// and it's important to set the VO to 0, so that the pcrel-ness is calculated correctly.
		insn->GetAddress()->SetVirtualOffset(0);	
			
		Relocation_t *reloc=new Relocation_t;
		reloc->SetOffset(0);
		reloc->SetType("pcrel");
		insn->GetRelocations().insert(reloc);
		firp->GetRelocations().insert(reloc);

		insn->Disassemble(disasm);
		if(getenv("VERBOSE_FIX_CALLS"))
			cout<<" Converted to: "<<disasm.CompleteInstr<<endl;
	}
}

void fix_safefr(FileIR_t* firp, Instruction_t *insn, UIntPtr virt_offset)
{
	/* if this has already been fixed, we can skip it */
	if(virt_offset==0 || virt_offset==-1)
		return;

	for(set<Relocation_t*>::iterator it=insn->GetRelocations().begin();
		it!=insn->GetRelocations().end();
		++it)
	{
		Relocation_t* reloc=*it;
		assert(reloc);
		if(string("safefr") == reloc->GetType())
		{
			AddressID_t* addr	=new AddressID_t;
			addr->SetFileID(insn->GetAddress()->GetFileID());
			firp->GetAddresses().insert(addr);
			insn->SetAddress(addr);
		}
	}
}


void fix_other_pcrel(FileIR_t* firp)
{

	for(
		set<Instruction_t*>::const_iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end(); 
		++it
	   )
	{
		Instruction_t* insn=*it;
		fix_other_pcrel(firp,insn, insn->GetAddress()->GetVirtualOffset());
		fix_safefr(firp,insn, insn->GetAddress()->GetVirtualOffset());
	}
}

//
// main rountine; convert calls into push/jump statements 
//
main(int argc, char* argv[])
{

	bool fix_all=false;
	bool do_eh_frame=true;

	if(argc<2)
	{
		cerr<<"Usage: fix_calls <id> [--fix-all | --no-fix-all ] [--eh-frame | --no-ehframe] "<<endl;
		cerr<<" --eh-frame " << endl;
		cerr<<" --no-eh-frame 		Use (or dont) the eh-frame section to be compatible with exception handling." << endl;
		cerr<<" --fix-all " << endl;
		cerr<<" --no-fix-all 		Convert (or don't) all calls to push/jmp pairs."<<endl;
		cerr<<" --fix-icalls 		Convert (or don't) indirect calls."<<endl;
		cerr<<" --no-fix-icalls 	Convert (or don't) indirect calls."<<endl;
		exit(-1);
	}

	for(int argc_iter=2; argc_iter<argc; argc_iter++)
	{
		if(strcmp("--fix-all", argv[argc_iter])==0)
		{
			fix_all=true;
		}
		else if(strcmp("--no-fix-all", argv[argc_iter])==0)
		{
			fix_all=false;
		}
		else if(strcmp("--eh-frame", argv[argc_iter])==0)
		{
			do_eh_frame=true;
		}
		else if(strcmp("--no-eh-frame", argv[argc_iter])==0)
		{
			do_eh_frame=false;
		}
		else if(strcmp("--fix-icalls", argv[argc_iter])==0)
		{
			opt_fix_icalls = true;
		}
		else if(strcmp("--no-fix-icalls", argv[argc_iter])==0)
		{
			opt_fix_icalls = false;
		}
		else
		{
			cerr<<"Unrecognized option: "<<argv[argc_iter]<<endl;
			exit(-1);
		}
	}
	if(getenv("FIX_CALLS_FIX_ALL_CALLS"))
		fix_all=true;

	VariantID_t *pidp=NULL;
	FileIR_t *firp=NULL;

	/* setup the interface to the sql server */
	BaseObj_t::SetInterface(&pqxx_interface);

	cout<<"Reading variant "<<string(argv[1])<<" from database." << endl;
	try 
	{

		pidp=new VariantID_t(atoi(argv[1]));
		cout<<"Fixing calls->push/jmp in variant "<<*pidp<< "." <<endl;

		assert(pidp->IsRegistered()==true);

                for(set<File_t*>::iterator it=pidp->GetFiles().begin();
                        it!=pidp->GetFiles().end();
                        ++it
                    )
                {
                        File_t* this_file=*it;
                        assert(this_file);

			// read the db  
			firp=new FileIR_t(*pidp,this_file);
	
			assert(firp && pidp);
	
			eh_frame_ranges.clear();
                        int elfoid=firp->GetFile()->GetELFOID();
                        pqxx::largeobject lo(elfoid);
                        lo.to_file(pqxx_interface.GetTransaction(),"readeh_tmp_file.exe");
                        EXEIO::exeio*    elfiop=new EXEIO::exeio;
                        elfiop->load((const char*)"readeh_tmp_file.exe");
                        EXEIO::dump::header(cout,*elfiop);
                        EXEIO::dump::section_headers(cout,*elfiop);
			// do eh_frame reading as required. 
			if(do_eh_frame)
        			read_ehframe(firp, elfiop);

			fix_all_calls(firp,true,fix_all);
			fix_other_pcrel(firp);
			firp->WriteToDB();

			cout<<"Done!"<<endl;
			delete firp;

		}
		cout<<"Writing variant "<<*pidp<<" back to database." << endl;
		pqxx_interface.Commit();


	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	delete pidp;
}






void range(virtual_offset_t a, virtual_offset_t b)
{
	// non-zero sized fde
	assert(a<b);

	RangeSet_t::iterator rangeiter=eh_frame_ranges.find(Range_t(a+1,a+1));
	assert(rangeiter==eh_frame_ranges.end());

	eh_frame_ranges.insert(Range_t(a+1,b));	// ranges are interpreted as (a,b]
}

bool possible_target(uintptr_t p, uintptr_t at, ibt_provenance_t prov)
{
	// used for LDSA
}

