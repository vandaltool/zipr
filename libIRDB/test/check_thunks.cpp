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

#include "check_thunks.hpp"


using namespace libIRDB;
using namespace std;

#define HIWORD(a) ((a)&0xFFFF0000)


/*
 * check_for_thunk_offsets - check non-function thunks for extra offsets 
 */
void check_for_thunk_offsets(FileIR_t* firp, int thunk_base)
{



	for(
		set<Instruction_t*>::iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end();
		++it
	   )
	{
		// if it has a targ and fallthrough (quick test) it might be a call 
		Instruction_t* insn=*it;
		DISASM d;
		insn->Disassemble(d);
	
		if(string(d.Instruction.Mnemonic)==string("add "))
		{
			// check that arg2 is a constant
			if(HIWORD(d.Argument2.ArgType)!=CONSTANT_TYPE+ABSOLUTE_)
				continue;

			string add_offset=string(d.Argument2.ArgMnemonic);

			int addoff=strtol(add_offset.c_str(),NULL,16);

			/* bounds check gently */
			if(0<addoff && addoff<100)
				continue;

			possible_target(thunk_base+addoff);
		}
		else if(string(d.Instruction.Mnemonic)==string("lea "))
		{
			assert (d.Argument2.ArgType==MEMORY_TYPE);

			/* no indexing please! */
			if(d.Argument2.Memory.IndexRegister!=0)
				continue;

			int leaoff=d.Argument2.Memory.Displacement;

			/* bounds check gently */
			if(0<leaoff && leaoff<100)
				continue;
			
			/* record that there's a possible target here */
			possible_target(thunk_base+leaoff);
			
		}
			
	}
}

void check_for_thunk_offsets(FileIR_t* firp, Instruction_t *thunk_insn, string reg, string offset)
{

	int thunk_base=thunk_insn->GetFallthrough()->GetAddress()->GetVirtualOffset()+
		strtol(offset.c_str(),NULL,16);
	int thunk_call_addr=thunk_insn->GetAddress()->GetVirtualOffset();
	int thunk_call_offset=strtol(offset.c_str(),NULL,16);

	/* don't check inserted thunk addresses */
	if(thunk_insn->GetAddress()->GetVirtualOffset()==0)
		return;

	check_for_thunk_offsets(firp,thunk_base);
}



/*
 * check_func_for_thunk_offsets - we know that insn represents a thunk call, with reg+offset as the constant.
 * check the rest of the function for offsets that might help form a code pointer.
 */
void check_func_for_thunk_offsets(Function_t *func, Instruction_t* thunk_insn,
	string reg, string offset)
{

	bool possible_target(uintptr_t p, uintptr_t at=0);


	int thunk_base=thunk_insn->GetFallthrough()->GetAddress()->GetVirtualOffset()+
		strtol(offset.c_str(),NULL,16);
	int thunk_call_addr=thunk_insn->GetAddress()->GetVirtualOffset();
	int thunk_call_offset=strtol(offset.c_str(),NULL,16);


	/* don't check inserted thunk addresses */
	if(thunk_insn->GetAddress()->GetVirtualOffset()==0)
		return;

	for(
		set<Instruction_t*>::iterator it=func->GetInstructions().begin();
		it!=func->GetInstructions().end();
		++it
	   )
	{
		// if it has a targ and fallthrough (quick test) it might be a call 
		Instruction_t* insn=*it;
		DISASM d;
		insn->Disassemble(d);
	
		if(string(d.Instruction.Mnemonic)==string("add "))
		{
			// check that arg2 is a constant
			if(HIWORD(d.Argument2.ArgType)!=CONSTANT_TYPE+ABSOLUTE_)
				continue;

			string add_offset=string(d.Argument2.ArgMnemonic);

			int addoff=strtol(add_offset.c_str(),NULL,16);

			/* bounds check gently */
			if(0<addoff && addoff<100)
				continue;

			/* record that there's a possible target here */
// 			cout <<"Possible thunk target (add): call:"<<thunk_call_addr<<" offset:"<<thunk_call_offset
//			     <<" addoff: " << addoff << " total: "<< (thunk_base+addoff)<<endl;
			possible_target(thunk_base+addoff);
		}
		else if(string(d.Instruction.Mnemonic)==string("lea "))
		{
			assert (d.Argument2.ArgType==MEMORY_TYPE);

			/* no indexing please! */
			if(d.Argument2.Memory.IndexRegister!=0)
				continue;

			int leaoff=d.Argument2.Memory.Displacement;

			/* bounds check gently */
			if(0<leaoff && leaoff<100)
				continue;
			
			/* record that there's a possible target here */
// 			cout <<"Possible thunk target (lea): call:"<<thunk_call_addr<<" offset:"<<thunk_call_offset
// 			     <<" leaoff: " << leaoff << " total: "<< (thunk_base+leaoff)<<endl;
			possible_target(thunk_base+leaoff);
			
		}
			
	}
}


/*
 * is_thunk_load - look for a mov reg<[esp], return reg in output parameter.
 */
bool is_thunk_load(Instruction_t* insn, string &reg)
{
	DISASM d;
	insn->Disassemble(d);

	if(string(d.Instruction.Mnemonic)!=string("mov "))
		return false;

	if(d.Argument2.ArgType!=MEMORY_TYPE || string(d.Argument2.ArgMnemonic)!=string("esp"))
		return false;

	reg=string(d.Argument1.ArgMnemonic);
	return true;
}

/*
 * is_ret - return trun if insn is a return 
 */
bool is_ret(Instruction_t* insn)
{
	DISASM d;
	insn->Disassemble(d);

	if(d.Instruction.BranchType!=RetType)
		return false;
	
	return true;
}


/* 
 * is_thunk_call - check if this instruction is a call to a thunk function, return the thunk function's reg.
 */
/* note: reg is output paramater */
bool is_thunk_call(Instruction_t* insn, string &reg)
{
	DISASM d;
	insn->Disassemble(d);

	/* not a call */
	if(d.Instruction.BranchType!=CallType)
		return false;

	/* no target in IRDB */
	if(insn->GetTarget()==NULL)
		return false;

	/* Target not the right type of load */
	if(!is_thunk_load(insn->GetTarget(),reg))
		return false;

	/* target has no FT? */
	if(!insn->GetTarget()->GetFallthrough())
		return false;

	/* target's FT is a return insn */
	if(!is_ret(insn->GetTarget()->GetFallthrough()))
		return false;

	return true;
}

/* 
 * is_thunk_add - Check the given instruction for an add of reg, return the constant K1 
 */
/* note: offset is an output parameter */
bool is_thunk_add(Instruction_t *insn, string reg, string &offset)
{
	DISASM d;
	insn->Disassemble(d);

	// make sure it's an add instruction 
	if(string(d.Instruction.Mnemonic)!=string("add "))
		return false;

	// check that it's an add of the proper reg
	if(string(d.Argument1.ArgMnemonic)!=reg)
		return false;

	// check that arg2 is a constant
	if(HIWORD(d.Argument2.ArgType)!=CONSTANT_TYPE+ABSOLUTE_)
		return false;

	offset=string(d.Argument2.ArgMnemonic);

	int intoff=strtol(offset.c_str(),NULL,16);

	/* bounds check gently */
	if(0<intoff && intoff<100)
		return false;

	return true;
}

/* 
 * check_func_for_thunk_calls - check this function for a thunk call (see check_for_thunks for description of thunk calls)
 */
void check_func_for_thunk_calls(Function_t* func)
{
	// for each insn in the func 
	for(
		set<Instruction_t*>::iterator it=func->GetInstructions().begin();
		it!=func->GetInstructions().end();
		++it
	   )
	{
		// if it has a targ and fallthrough (quick test) it might be a call 
		Instruction_t* insn=*it;
		/* check if we might be calling a thunk */
		if(insn->GetFallthrough() && insn->GetTarget())
		{

			// check for a call, followed by an add of reg (note the output params of reg and offset)
			string reg,offset;
			if(is_thunk_call(insn,reg) && 
				is_thunk_add(insn->GetFallthrough(),reg,offset))
			{
				check_func_for_thunk_offsets(func,insn,reg,offset);
			}
		}
	}
}



void check_non_funcs_for_thunks(FileIR_t *firp)
{
	// for each insn in the func 
	for(
		set<Instruction_t*>::iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end();
		++it
	   )
	{
		// if it has a targ and fallthrough (quick test) it might be a call 
		Instruction_t* insn=*it;

		/* these instructions/thunks are checked with the functions */

		/* check if we might be calling a thunk */
		if(insn->GetFallthrough() && insn->GetTarget())
		{

			// check for a call, followed by an add of reg (note the output params of reg and offset)
			string reg,offset;
			if(is_thunk_call(insn,reg) && 
				is_thunk_add(insn->GetFallthrough(),reg,offset))
			{
				cout<<"Found non-function thunk at "<<	
					insn->GetAddress()->GetVirtualOffset()<<endl;
				check_for_thunk_offsets(firp,insn,reg,offset);
			}
		}
	}
}


/* 
 * check_for_thunks - 
 * 
 * check the program (file) for this pattern:
 * 
 *	 		call   ebx_thunk
 *  	L1:		add    K1,%ebx
 *
 * 	ebx_thunk:	mov ebx <- [esp]
 * 			ret
 * 
 *  If found, check the function for L1+K1+K2 (where K2 is any constant in the function)
 *  If L1+k1+k2 is found, and points at a code address (outside this function?), mark it as an indirect branch target.
 * 
 */
void check_for_thunks(FileIR_t* firp, const std::set<int>&  thunk_bases)
{
	/* thunk bases is the module start's found for this firp */

	for(set<int>::iterator it=thunk_bases.begin(); it!=thunk_bases.end(); ++it)
	{
		int offset=*it;
		check_for_thunk_offsets(firp,offset);
	}
#if 0
	for(
		set<Function_t*>::iterator it=firp->GetFunctions().begin();
		it!=firp->GetFunctions().end();
		++it
	   )
	{
		Function_t* func=*it;

		if(getenv("THUNK_VERBOSE"))
		{
			cout<<"Checking for thunks in "<<func->GetName()<<endl;
		}
		check_func_for_thunk_calls(func);

	}

	check_non_funcs_for_thunks(firp);
#endif
}

void find_all_module_starts(FileIR_t* firp, set<int> &thunk_bases)
{
	thunk_bases.clear();

	// for each insn in the func 
	for(
		set<Instruction_t*>::iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end();
		++it
	   )
	{
		// if it has a targ and fallthrough (quick test) it might be a call 
		Instruction_t* insn=*it;

		/* check if we might be calling a thunk */
		if(insn->GetFallthrough() && insn->GetTarget())
		{
			// check for a call, followed by an add of reg (note the output params of reg and offset)
			string reg,offset;
			if(is_thunk_call(insn,reg) && 
				is_thunk_add(insn->GetFallthrough(),reg,offset))
			{
				int thunk_base=insn->GetFallthrough()->GetAddress()->GetVirtualOffset()+ 
					strtol(offset.c_str(),NULL,16);
				if(thunk_bases.find(thunk_base)==thunk_bases.end())
					cout<<"Found new thunk at "<<insn->GetAddress()->GetVirtualOffset()<<" with base: "<<hex<<thunk_base<<endl;
				thunk_bases.insert(thunk_base);
			}
		}
	}
}
