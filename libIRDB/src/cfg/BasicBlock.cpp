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


#include <map>
#include <libIRDB-core.hpp>
#include <libIRDB-cfg.hpp>
#include <utils.hpp>

using namespace libIRDB;
using namespace std;


BasicBlock_t::BasicBlock_t() 
	: is_exit_block(false)
{
}



void BasicBlock_t::BuildBlock
	(
		Function_t* func, 
		Instruction_t* insn, 
		const map<Instruction_t*,BasicBlock_t*> &insn2block_map
	)
{
	assert(insn);
	/* loop through the instructions for this block */
	while(insn)
	{
		/* insert this instruction */
		instructions.push_back(insn);

		Instruction_t* target_insn=insn->GetTarget();
		Instruction_t* ft_insn=insn->GetFallthrough();

		/* determine if there's a target block */
		BasicBlock_t* target_block=NULL;
		if (target_insn && is_in_container(insn2block_map,target_insn)) 
			target_block=find_map_object(insn2block_map,target_insn);

		/* determine if there's a ft block */
		BasicBlock_t* ft_block=NULL;
		if(ft_insn && is_in_container(insn2block_map,ft_insn))
			ft_block=find_map_object(insn2block_map,ft_insn);

		/* if there's a target block, insert it into the appropriate sets */
		if(target_block)
		{
			target_block->GetPredecessors().insert(this);
			successors.insert(target_block);
		}

		/* This is also the end of the block if this is a function exit instruction */
		if(insn->IsFunctionExit()) 
		{
			is_exit_block=true;
		}

		// handle fixed-call fallthroughs.
		for_each(insn->GetRelocations().begin(), insn->GetRelocations().end(), [this,&insn2block_map](Relocation_t* reloc)
		{
			// and has a reloc that's a pcrel with a WRT object 
			// possible for a call to have a null fallthrouth (and consequently null WRT)
			// becauase the call may be the last insn in a section, etc.
			if( reloc->GetType()==string("fix_call_fallthrough") && reloc->GetWRT()!=NULL) 
			{
				assert(reloc->GetWRT()!=NULL);
				Instruction_t* fix_call_fallthrough_insn=dynamic_cast<Instruction_t*>(reloc->GetWRT());
				assert(fix_call_fallthrough_insn);

				// this block has a fallthrough to the return block.
				if(is_in_container(insn2block_map,fix_call_fallthrough_insn))
				{
					BasicBlock_t* fix_call_fallthrough_blk=find_map_object(insn2block_map,fix_call_fallthrough_insn);
					successors.insert(fix_call_fallthrough_blk);
					fix_call_fallthrough_blk->GetPredecessors().insert(this);
				}

				is_exit_block=false;
			}
		});


		/* if there's a fallthrough block, insert it into the appropriate sets */
		if(ft_block)
		{
			ft_block->GetPredecessors().insert(this);
			successors.insert(ft_block);
		}
			
		/* this is the end of the block if there's a target instruction */
		if(target_insn)
			break;

		/* or if there is a fallthrough block already built */
		if(ft_block)
			break;

		/* check for a fallthrough out of the function */
		if(ft_insn && !is_in_container(func->GetInstructions(),ft_insn))
			break;


		/* otherwise, move to the fallthrough */
		insn=ft_insn;
	}

	// deal with IB targets for the end of the block

	insn=instructions[instructions.size()-1]; // get last instruction.
	assert(insn);
	if(insn->GetIBTargets())
	{
		for_each(insn->GetIBTargets()->begin(), insn->GetIBTargets()->end(), [this,&insn2block_map,func](Instruction_t* target)
		{
			if(is_in_container(insn2block_map,target) && target!=func->GetEntryPoint())	// don't link calls to the entry block.
			{
				BasicBlock_t* target_block=find_map_object(insn2block_map,target);
				target_block->GetPredecessors().insert(this);
				successors.insert(target_block);
			}
		});
	}
	
}


std::ostream& libIRDB::operator<<(std::ostream& os, const BasicBlock_t& block)
{
	os<<block.is_exit_block;
	os<<"\t ---- Starting block print -----" <<endl;
	for(auto i=0U;i<block.instructions.size();i++)
	{
		const auto insn=block.instructions[i];
		os<<"\t Instruction "<<std::dec<<i<<" at " << std::hex << insn->GetAddress()->GetVirtualOffset() << " with id " << std::dec << insn->GetBaseID() << " " << insn->GetComment() << endl;
	}
	os<<"\t ---- done block print -----" <<endl;
	os<<endl;

	return os;
}


bool BasicBlock_t::EndsInBranch() 
{
	const auto branch=instructions[instructions.size()-1];	
	assert(branch);

	const auto d=DecodedInstruction_t(branch);
	return d.isBranch();

	
}
bool BasicBlock_t::EndsInIndirectBranch() 
{
	const auto *branch=instructions[instructions.size()-1];	
	assert(branch);

	const auto d=DecodedInstruction_t(branch);

	if(d.isReturn())
		return true;
	if(d.isUnconditionalBranch() || d.isCall())
	{
		if(!d.getOperand(0).isConstant())
			/* not a constant type */
			return true;
		return false;
		
	}
	return false;
}
bool BasicBlock_t::EndsInConditionalBranch() 
{
	if(!EndsInBranch())
		return false;
	const auto branch=instructions[instructions.size()-1];	
	assert(branch);
	const auto d=DecodedInstruction_t(branch);

	return d.isConditionalBranch(); 
}

Instruction_t* BasicBlock_t::GetBranchInstruction()

{
	if(!EndsInBranch())
		return NULL;

	auto branch=instructions[instructions.size()-1];	
	return branch;
}

