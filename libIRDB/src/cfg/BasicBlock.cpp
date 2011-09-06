
#include <map>
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
		if(target_insn || ft_block)
			break;

		/* This is also the end of the block if this is a function exit instruction */
		if(insn->IsFunctionExit()) 
			break;

		/* otherwise, move to the fallthrough */
		insn=ft_insn;
	}
	
}


std::ostream& libIRDB::operator<<(std::ostream& os, const BasicBlock_t& block)
{
	int i;
	os<<block.is_exit_block;
	os<<"\t ---- Starting block print -----" <<endl;
	for(i=0;i<block.instructions.size();i++)
	{
		Instruction_t* insn=block.instructions[i];
		os<<"\t Instruction "<<std::dec<<i<<" at " << std::hex << insn->GetAddress()->GetVirtualOffset() << " with id " << std::dec << insn->GetBaseID() << " " << insn->GetComment() << endl;
	}
	os<<"\t ---- done block print -----" <<endl;
	os<<endl;

	return os;
}

