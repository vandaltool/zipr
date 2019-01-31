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






#include <irdb-core>
#include <utils.hpp>
#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <elf.h>

#include "check_thunks.hpp"
#include "fill_in_indtargs.hpp"


using namespace IRDB_SDK;
using namespace std;

#define HIWORD(a) ((a)&0xFFFF0000)


/*
 * check_for_thunk_offsets - check non-function thunks for extra offsets 
 */
void check_for_thunk_offsets(FileIR_t* firp, VirtualOffset_t thunk_base)
{

	for(auto insn : firp->getInstructions())
	{
		const auto d=DecodedInstruction_t::factory(insn);
		if(d->getMnemonic()=="add")
		{
			// check that arg2 is a constant
			if(!d->getOperand(1)->isConstant())
				continue;

			auto addoff=d->getOperand(1)->getConstant(); 

			/* bounds check gently */
			if(0<addoff && addoff<100)
				continue;

			possible_target(thunk_base+addoff, 0, ibt_provenance_t::ibtp_text);
		}
		// else if(string(d.Instruction.Mnemonic)==string("lea "))
		if(d->getMnemonic()=="lea")
		{
			assert (d->getOperand(1)->isMemory());

			/* no indexing please! */
			if(d->getOperand(1)->hasIndexRegister())
				continue;

			auto leaoff=d->getOperand(1)->getMemoryDisplacement();

			/* bounds check gently */
			if(0<leaoff && leaoff<100)
				continue;
			
			/* record that there's a possible target here */
			possible_target(thunk_base+leaoff, 0, ibt_provenance_t::ibtp_text);
			
		}
			
	}
}

void check_for_thunk_offsets(FileIR_t* firp, Instruction_t *thunk_insn, string reg, uint64_t offset)
{

	auto thunk_base=thunk_insn->getFallthrough()->getAddress()->getVirtualOffset()+offset;

	/* don't check inserted thunk addresses */
	if(thunk_insn->getAddress()->getVirtualOffset()==0)
		return;

	check_for_thunk_offsets(firp,thunk_base);
}



/*
 * check_func_for_thunk_offsets - we know that insn represents a thunk call, with reg+offset as the constant.
 * check the rest of the function for offsets that might help form a code pointer.
 */
void check_func_for_thunk_offsets(Function_t *func, Instruction_t* thunk_insn,
	string reg, uint64_t offset)
{



	auto thunk_base=thunk_insn->getFallthrough()->getAddress()->getVirtualOffset()+offset;


	/* don't check inserted thunk addresses */
	if(thunk_insn->getAddress()->getVirtualOffset()==0)
		return;

	for(auto insn : func->getInstructions())
	{
		// if it has a targ and fallthrough (quick test) it might be a call 
		const auto d=DecodedInstruction_t::factory(insn);
	
		if(d->getMnemonic()=="add")
		{
			// check that arg2 is a constant
			if(!d->getOperand(1)->isConstant())
				continue;

			auto addoff=d->getOperand(1)->getConstant(); 

			/* bounds check gently */
			if(0<addoff && addoff<100)
				continue;

			/* record that there's a possible target here */
// 			cout <<"Possible thunk target (add): call:"<<thunk_call_addr<<" offset:"<<thunk_call_offset
//			     <<" addoff: " << addoff << " total: "<< (thunk_base+addoff)<<endl;
			possible_target(thunk_base+addoff, 0, ibt_provenance_t::ibtp_text);
		}
		else if(d->getMnemonic()=="lea")
		{
			assert (d->getOperand(1)->isMemory()); 

			/* no indexing please! */
			if(!d->getOperand(1)->hasIndexRegister())
				continue;

			VirtualOffset_t leaoff=d->getOperand(1)->getMemoryDisplacement();

			/* bounds check gently */
			if(0<leaoff && leaoff<100)
				continue;
			
			/* record that there's a possible target here */
// 			cout <<"Possible thunk target (lea): call:"<<thunk_call_addr<<" offset:"<<thunk_call_offset
// 			     <<" leaoff: " << leaoff << " total: "<< (thunk_base+leaoff)<<endl;
			possible_target(thunk_base+leaoff, 0, ibt_provenance_t::ibtp_text);
			
		}
			
	}
}


/*
 * is_thunk_load - look for a mov reg<[esp], return reg in output parameter.
 */
bool is_thunk_load(Instruction_t* insn, string &reg)
{
	const auto d=DecodedInstruction_t::factory(insn);

	if(d->getMnemonic()!="mov")
		return false;

	if(!d->getOperand(1)->isMemory() || d->getOperand(1)->getString()!="esp")
		return false;

	reg=d->getOperand(0)->getString();
	return true;
}

/*
 * is_ret - return true if insn is a return 
 */
bool is_ret(Instruction_t* insn)
{
	const auto d=DecodedInstruction_t::factory(insn);
	return d->isReturn(); 
}

/*
 * is_pop - return true if insn is a return
 */
bool is_pop(Instruction_t* insn, string &reg)
{
	const auto d=DecodedInstruction_t::factory(insn);

        if(d->getMnemonic()!="pop")
                return false;

	reg=d->getOperand(0)->getString();
        return true;
}



/* 
 * is_thunk_call - check if this instruction is a call to a thunk function, return the thunk function's reg.
 */
/* note: reg is output paramater */
bool is_thunk_call(Instruction_t* insn, string &reg)
{
	const auto d=DecodedInstruction_t::factory(insn);

	/* not a call */
	if(!d->isCall())
		return false;

	/* no target in IRDB */
	if(insn->getTarget()==NULL)
		return false;

	/* Target not the right type of load */
	if(!is_thunk_load(insn->getTarget(),reg))
		return false;

	/* target has no FT? */
	if(!insn->getTarget()->getFallthrough())
		return false;

	/* target's FT is a return insn */
	if(!is_ret(insn->getTarget()->getFallthrough()))
		return false;

	return true;
}

bool is_thunk_call_type2(Instruction_t* insn, string &reg, Instruction_t** newinsn)
{
	const auto d=DecodedInstruction_t::factory(insn);

	/* not a call */
	if(!d->isCall())
		return false;

	/* no target in IRDB */
	if(insn->getTarget()==NULL)
		return false;

	if(insn->getTarget() != insn->getFallthrough())
		return false;

	/* target's FT is a return insn */
	if(!is_pop(insn->getTarget(), reg))
		return false;

	*newinsn=insn->getTarget()->getFallthrough();

	return true;
}

/* 
 * is_thunk_add - Check the given instruction for an add of reg, return the constant K1 
 */
/* note: offset is an output parameter */
bool is_thunk_add(Instruction_t *insn, string reg, uint64_t &offset)
{
	const auto d=DecodedInstruction_t::factory(insn);

	// make sure it's an add instruction 
	if(d->getMnemonic()!="add")
		return false;

	// check that it's an add of the proper reg
	if(d->getOperand(0)->getString()!=reg)
		return false;

	// check that arg2 is a constant
	if(!d->getOperand(1)->isConstant())
		return false;

	offset=d->getOperand(1)->getConstant();

	auto intoff=d->getOperand(1)->getConstant(); 

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
	for(auto insn : func->getInstructions())
	{
		// if it has a targ and fallthrough (quick test) it might be a call 
		/* check if we might be calling a thunk */
		if(insn->getFallthrough() && insn->getTarget())
		{

			// check for a call, followed by an add of reg (note the output params of reg and offset)
			string reg;
			uint64_t offset=0;
			if(is_thunk_call(insn,reg) && 
				is_thunk_add(insn->getFallthrough(),reg,offset))
			{
				check_func_for_thunk_offsets(func,insn,reg,offset);
			}
		}
	}
}



void check_non_funcs_for_thunks(FileIR_t *firp)
{
	// for each insn in the func 
	for(auto insn :  firp->getInstructions())
	{
		// if it has a targ and fallthrough (quick test) it might be a call 

		/* these instructions/thunks are checked with the functions */

		/* check if we might be calling a thunk */
		if(insn->getFallthrough() && insn->getTarget())
		{

			// check for a call, followed by an add of reg (note the output params of reg and offset)
			string reg; 
			uint64_t offset=0;
			if(is_thunk_call(insn,reg) && 
				is_thunk_add(insn->getFallthrough(),reg,offset))
			{
				cout<<"Found non-function thunk at "<<	
					insn->getAddress()->getVirtualOffset()<<endl;
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
void check_for_thunks(FileIR_t* firp, const std::set<VirtualOffset_t>&  thunk_bases)
{
	/* thunk bases is the module start's found for this firp */

	cout<<"Starting check for thunks"<<endl;

	for(auto offset : thunk_bases)
		check_for_thunk_offsets(firp,offset);
}

void find_all_module_starts(FileIR_t* firp, set<VirtualOffset_t> &thunk_bases)
{
	thunk_bases.clear();

	cout<<"Finding thunk bases"<<endl;

	// for each insn in the func 
	for(auto insn : firp->getInstructions())
	{
		// if it has a targ and fallthrough (quick test) it might be a call 
		/* check if we might be calling a thunk */
		if(insn->getFallthrough() && insn->getTarget())
		{
			// check for a call, followed by an add of reg (note the output params of reg and offset)
			string reg; 
			uint64_t offset=0;
			if(is_thunk_call(insn,reg) && 
				is_thunk_add(insn->getFallthrough(),reg,offset))
			{
				auto thunk_base=insn->getFallthrough()->getAddress()->getVirtualOffset()+ offset;
				if(thunk_bases.find(thunk_base)==thunk_bases.end())
					cout<<"Found new thunk at "<<insn->getAddress()->getVirtualOffset()<<" with base: "<<hex<<thunk_base<<endl;
				thunk_bases.insert(thunk_base);
			}
			Instruction_t* newinsn=NULL;
			if(is_thunk_call_type2(insn,reg,&newinsn))
			{
				if(newinsn && is_thunk_add(newinsn,reg,offset))
				{
					VirtualOffset_t thunk_base=insn->getFallthrough()->getAddress()->getVirtualOffset()+ offset;
					if(thunk_bases.find(thunk_base)==thunk_bases.end())
						cout<<"Found new thunk at "<<insn->getAddress()->getVirtualOffset()<<" with base: "<<hex<<thunk_base<<endl;
					thunk_bases.insert(thunk_base);
				}
			}
		}
	}
}
