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

#define ALLOF(a) begin(a),end(a)

/*
 *  FindTargets - locate all possible instructions that are the target of a jump instruction
 */
static InstructionSet_t FindBlockStarts(Function_t* func) 
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


void ControlFlowGraph_t::alloc_blocks(const InstructionSet_t &starts, map<Instruction_t*,BasicBlock_t*>& insn2block_map)
{
	/* create a basic block for each instruction that starts a block */
	for(const auto &insn : starts)
	{
		if(is_in_container(insn2block_map,insn)) // already allocated 
			continue;

		auto  newblock=new BasicBlock_t;

		assert( insn && newblock );

		blocks.insert(newblock);
		insn2block_map[insn]=newblock;
	}
}

void ControlFlowGraph_t::build_blocks(const map<Instruction_t*,BasicBlock_t*>& insn2block_map)
{

	/* Ask the basic block to set the fields for each block that need to be set */
	for(const auto &it : insn2block_map)
	{
		const auto insn=it.first;
		const auto block=it.second;

		if(block->GetInstructions().size()>0) // already built
			continue;

		assert(insn && block);

		block->BuildBlock(insn, insn2block_map);

	}

}

void ControlFlowGraph_t::find_unblocked_instructions(InstructionSet_t &starts, Function_t* func)
{
	auto mapped_instructions=InstructionSet_t();
	auto missed_instructions=InstructionSet_t();
	for(const auto block : GetBlocks())
		mapped_instructions.insert(ALLOF(block->GetInstructions()));

	auto my_inserter=inserter(missed_instructions,missed_instructions.end());
	set_difference(ALLOF(func->GetInstructions()), ALLOF(mapped_instructions), my_inserter);
	starts.insert(ALLOF(missed_instructions));
}



void ControlFlowGraph_t::Build(Function_t* func)
{
	auto starts=FindBlockStarts(func);

	auto insn2block_map=map<Instruction_t*,BasicBlock_t*> ();

	alloc_blocks(starts, insn2block_map);
	build_blocks(insn2block_map);
	/* record the entry block */
	if(func->GetEntryPoint())
		entry=insn2block_map[func->GetEntryPoint()];

	/* most functions are done now. */
	/* however, if a function has a (direct) side entrance, 
	 * some code may appear unreachable and not be placed in 
	 * a block -- here, we detect that code and create a 
	 * new basic block for every instruction, as any may have a side entrance
	 */ 
	/* note:  side entrances may miss a block start */
	/* in code that appears reachable from the entrance?! */
	find_unblocked_instructions(starts, func);
	alloc_blocks(starts, insn2block_map);
	build_blocks(insn2block_map);
}

// returns true iff there's an edge from <p_src> to <p_tgt> in the CFG
bool ControlFlowGraph_t::HasEdge(BasicBlock_t *p_src, BasicBlock_t *p_tgt) const
{
	const auto src_exists = blocks.find(p_src) != blocks.end();
	const auto tgt_exists = blocks.find(p_tgt) != blocks.end();

	if (!src_exists || !tgt_exists) return false;

	const auto successors = p_src->GetSuccessors();
	return successors.find(p_tgt) != successors.end();
}

CFG_EdgeType ControlFlowGraph_t::GetEdgeType(const BasicBlock_t *p_src, const BasicBlock_t *p_tgt) const
{
	const auto last_in_src = p_src->GetInstructions()[p_src->GetInstructions().size()-1];
	const auto first_in_tgt = p_tgt->GetInstructions()[0];

	auto edgeType = CFG_EdgeType();

	if (last_in_src->GetFallthrough() == first_in_tgt)
	{
		edgeType.insert(CFG_FallthroughEdge);
	}

	if (last_in_src->GetTarget() == first_in_tgt)
	{
		edgeType.insert(CFG_TargetEdge);
	}

	if (edgeType.size() == 0)
	{
		edgeType.insert(CFG_IndirectEdge);
	}

	return edgeType;
}

/*
 *  output operator
 */
ostream& libIRDB::operator<<(ostream& os, const ControlFlowGraph_t& cfg)
{
	int i=0;

	map<BasicBlock_t*,int> blk_numbers;
	for(
		set<BasicBlock_t*>::const_iterator it=cfg.blocks.begin();
		it!=cfg.blocks.end();
		++it
	   )
	{
			blk_numbers[*it]=i++;
	}
	

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
		os<<"block "<<std::dec<<blk_numbers[block]<<endl;
		os<<"Successors: ";
		for_each(block->GetSuccessors().begin(), block->GetSuccessors().end(), [&](BasicBlock_t* succ)
		{
			os<<blk_numbers[succ]<<", ";
			
		});
		os<<endl;
		os<<"Predecessors: ";
		for_each(block->GetPredecessors().begin(), block->GetPredecessors().end(), [&](BasicBlock_t* pred)
		{
			os<<blk_numbers[pred]<<", ";
		});
		os<<endl;
		os << *block;
	}

	return os;
}

