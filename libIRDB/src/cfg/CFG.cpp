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
static InstructionSet_t FindBlockStarts(IRDB_SDK::Function_t* func) 
{

	InstructionSet_t targets;
	InstructionSet_t found_fallthrough_to;

	if(func->getEntryPoint())
		/* the entry point of the function is a target instruction for this CFG */
		targets.insert(func->getEntryPoint());

	/* for each instruction, decide if it's a block start based on whether or not 
	 * it can be indirectly branched to.  Also mark direct targets as block starts.
	 */
	for(auto it=func->getInstructions().begin();
		it!=func->getInstructions().end();
		++it
	   )
	{
		/* Get the instruction */
		auto insn=*it;

		/* If this instruction might be an indirect branch target, then mark it as a block start */
		if(insn->getIndirectBranchTargetAddress())
		{
			targets.insert(insn);
		}

		/* get target and fallthrough */
		auto target=insn->getTarget();
		auto ft=insn->getFallthrough();

		/* if this instruction has a target, and the target is in this function, mark the target as a block start */
		if(target && is_in_container(func->getInstructions(), target))
			targets.insert(target);

		/* there is a target, and a failthrough, and the fallthrough is in this function */
		if(target && ft && is_in_container(func->getInstructions(), ft))
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

ControlFlowGraph_t::ControlFlowGraph_t(IRDB_SDK::Function_t* func) :
	entry(NULL), function(dynamic_cast<Function_t*>(func))
{
	Build(func);	
}


void ControlFlowGraph_t::alloc_blocks(const IRDB_SDK::InstructionSet_t &starts, map<IRDB_SDK::Instruction_t*,BasicBlock_t*>& insn2block_map)
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

void ControlFlowGraph_t::build_blocks(const map<IRDB_SDK::Instruction_t*,BasicBlock_t*>& insn2block_map)
{

	/* Ask the basic block to set the fields for each block that need to be set */
	for(const auto &it : insn2block_map)
	{
		const auto insn=it.first;
		const auto block=it.second;

		if(block->getInstructions().size()>0) // already built
			continue;

		assert(insn && block);

		block->BuildBlock(insn, insn2block_map);

	}

}

void ControlFlowGraph_t::find_unblocked_instructions(IRDB_SDK::InstructionSet_t &starts, IRDB_SDK::Function_t* func)
{
	auto mapped_instructions=InstructionSet_t();
	auto missed_instructions=InstructionSet_t();
	for(const auto block : GetBlocks())
		mapped_instructions.insert(ALLOF(block->getInstructions()));

	auto my_inserter=inserter(missed_instructions,missed_instructions.end());
	set_difference(ALLOF(func->getInstructions()), ALLOF(mapped_instructions), my_inserter);
	starts.insert(ALLOF(missed_instructions));
}



void ControlFlowGraph_t::Build(IRDB_SDK::Function_t* func)
{
	auto starts=FindBlockStarts(func);

	auto insn2block_map=map<IRDB_SDK::Instruction_t*,BasicBlock_t*> ();

	alloc_blocks(starts, insn2block_map);
	build_blocks(insn2block_map);
	/* record the entry block */
	if(func->getEntryPoint())
		entry=insn2block_map[func->getEntryPoint()];

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
bool ControlFlowGraph_t::hasEdge(IRDB_SDK::BasicBlock_t *p_src, IRDB_SDK::BasicBlock_t *p_tgt) const
{
	const auto src_exists = blocks.find(p_src) != blocks.end();
	const auto tgt_exists = blocks.find(p_tgt) != blocks.end();

	if (!src_exists || !tgt_exists) return false;

	const auto successors = p_src->getSuccessors();
	return successors.find(p_tgt) != successors.end();
}

IRDB_SDK::CFGEdgeType_t ControlFlowGraph_t::getEdgeType(const IRDB_SDK::BasicBlock_t *p_src, const IRDB_SDK::BasicBlock_t *p_tgt) const
{
	const auto last_in_src = p_src->getInstructions()[p_src->getInstructions().size()-1];
	const auto first_in_tgt = p_tgt->getInstructions()[0];

	auto edgeType = IRDB_SDK::CFGEdgeType_t();

	if (last_in_src->getFallthrough() == first_in_tgt)
	{
		edgeType.insert(IRDB_SDK::cetFallthroughEdge);
	}

	if (last_in_src->getTarget() == first_in_tgt)
	{
		edgeType.insert(IRDB_SDK::cetTargetEdge);
	}

	if (edgeType.size() == 0)
	{
		edgeType.insert(IRDB_SDK::cetIndirectEdge);
	}

	return edgeType;
}

/*
 *  output operator
 */
ostream& IRDB_SDK::operator<<(ostream& os, const IRDB_SDK::ControlFlowGraph_t& cfg)
{
	cfg.dump(os);
	return os;
}

void ControlFlowGraph_t::dump(ostream& os) const
{
	int i=0;

	auto blk_numbers = map<IRDB_SDK::BasicBlock_t*,int>();
	for(auto blk : getBlocks() ) 
	{
			blk_numbers[blk]=i++;
	}
	

	for(auto block : getBlocks())
	{
		if(block==getEntry())
			os<<"**** Entry    ";
		else
			os<<"---- NotEntry ";
		os<<"block "<<std::dec<<blk_numbers[block]<<endl;
		os<<"Successors: ";
		for(auto succ : block->getSuccessors())
		{
			os<<blk_numbers[succ]<<", ";
			
		};
		os<<endl;
		os<<"Predecessors: ";
		for(auto pred : block->getPredecessors())
		{
			os<<blk_numbers[pred]<<", ";
		};
		os<<endl;
		os << *block;
	}

}

unique_ptr<IRDB_SDK::ControlFlowGraph_t> IRDB_SDK::ControlFlowGraph_t::factory(IRDB_SDK::Function_t* func)
{
	return unique_ptr<IRDB_SDK::ControlFlowGraph_t>(new libIRDB::ControlFlowGraph_t(func));
}

