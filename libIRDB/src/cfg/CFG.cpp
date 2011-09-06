
#include <libIRDB-cfg.hpp>
#include <utils.hpp>

using namespace std;
using namespace libIRDB;

/*
 *  FindTargets - locate all possible instructions that are the target of a jump instruction
 */
static set<Instruction_t*> FindBlockStarts(Function_t* func) 
{

	set<Instruction_t*> targets;

	/* the entry point of the function is a target instruction for this CFG */
	targets.insert(func->GetEntryPoint());

	/* for each instruction, decide if it's a block start based on whether or not 
	 * it can be indirectly branched to.  Also mark direct targets as block starts.
	 */
	for(set<Instruction_t*>::iterator it=func->GetInstructions().begin();
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

