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


#include <libIRDB-cfg.hpp>
#include <utils.hpp>

using namespace std;
using namespace libIRDB;

/*
 *  FindTargets - locate all possible instructions that are the target of a jump instruction
 */
static set<Instruction_t*> FindBlockStarts(Function_t* func) 
{

	InstructionSet_t targets;
	InstructionSet_t found_fallthrough_to;

	if(func->GetEntryPoint())
		/* the entry point of the function is a target instruction for this CFG */
		targets.insert(func->GetEntryPoint());

	/* for each instruction, decide if it's a block start based on whether or not 
	 * it can be indirectly branched to.  Also mark direct targets as block starts.
	 */
	for(InstructionSet_t::iterator it=func->GetInstructions().begin();
		it!=func->GetInstructions().end();
		++it
	   )
	{
		/* Get the instruction */
		Instruction_t* insn=*it;

		/* If this instruction might be an indirect branch target, then mark it as a block start */
		if(insn->GetIndirectBranchTargetAddress())
		{
			targets.insert(insn);
		}

		/* get target and fallthrough */
		Instruction_t* target=insn->GetTarget();
		Instruction_t* ft=insn->GetFallthrough();

		/* if this instruction has a target, and the target is in this function, mark the target as a block start */
		if(target && is_in_container(func->GetInstructions(), target))
			targets.insert(target);

		/* there is a target, and a failthrough, and the fallthrough is in this function */
		if(target && ft && is_in_container(func->GetInstructions(), ft))
			targets.insert(ft);

		// we already found a fallthrough to ft, so we have 2+ fallthroughs to ft.  mark it as a control flow merge.
		if( ft && is_in_container(found_fallthrough_to, ft))
			targets.insert(ft);
		
		// if there is a ft, mark that we've seen it now.	
		if(ft)
			found_fallthrough_to.insert(ft);
		
	}

	return targets;
}




ControlFlowGraph_t::ControlFlowGraph_t(Function_t* func) :
	entry(NULL), function(func)
{
	Build(func);	
}

void ControlFlowGraph_t::Build(Function_t* func)
{
	set<Instruction_t*> starts=FindBlockStarts(func);

	map<Instruction_t*,BasicBlock_t*> insn2block_map;

	/* create a basic block for each instruction that starts a block */
	for(	set<Instruction_t*>::const_iterator it=starts.begin();
		it!=starts.end();
		++it
	   )
	{
		Instruction_t* insn=*it;
		BasicBlock_t* newblock=new BasicBlock_t;

		/* record the entry block */
		if(insn==func->GetEntryPoint())
			entry=newblock;

		assert( insn && newblock );

		blocks.insert(newblock);
		insn2block_map[insn]=newblock;
	}

	/* Ask the basic block to set the fields for each block that need to be set */
	for(	map<Instruction_t*,BasicBlock_t*>::const_iterator it=insn2block_map.begin();
		it!=insn2block_map.end();
		++it
	   )
	{
		Instruction_t* insn=(*it).first;
		BasicBlock_t* block=(*it).second;

		assert(insn && block);

		block->BuildBlock(func, insn, insn2block_map);

	}



}

/*
 *  output operator
 */
ostream& libIRDB::operator<<(ostream& os, const ControlFlowGraph_t& cfg)
{
	int i=0;
	
	for(
		set<BasicBlock_t*>::const_iterator it=cfg.blocks.begin();
		it!=cfg.blocks.end();
		++it
	   )
	{
		BasicBlock_t *block=*it;

		if(block==cfg.GetEntry())
			os<<"**** Entry    ";
		else
			os<<"---- NotEntry ";
		os<<"block "<<std::dec<<i<<endl;
		i++;

		os << *block;
		
	}

	return os;
}

