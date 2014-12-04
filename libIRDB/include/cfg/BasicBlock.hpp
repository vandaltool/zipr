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



class BasicBlock_t
{

	public:
		BasicBlock_t();

		bool GetIsExitBlock() { return is_exit_block; }
		void SetIsExitBlock(bool is_exit) { is_exit_block=is_exit; }

		std::vector<Instruction_t*>& GetInstructions() { return instructions; }
		std::set<BasicBlock_t*>&     GetPredecessors() { return predecessors; }
		std::set<BasicBlock_t*>&     GetSuccessors()   { return successors; }
		std::set<BasicBlock_t*>&     GetIndirectTargets() { return indirect_targets; }

		BasicBlock_t* GetFallthrough();
		BasicBlock_t* GetTarget();
		bool EndsInBranch();
		bool EndsInIndirectBranch();
		bool EndsInConditionalBranch();
		Instruction_t* GetBranchInstruction();

		void BuildBlock(Function_t* func,
                		Instruction_t* insn,
                		const std::map<Instruction_t*,BasicBlock_t*> &insn2block_map
        			);


	private:

		std::vector<Instruction_t*> instructions;
		std::set<BasicBlock_t*> predecessors;
		std::set<BasicBlock_t*> successors;
		std::set<BasicBlock_t*> indirect_targets;
		bool is_exit_block;

	friend std::ostream& operator<<(std::ostream& os, const BasicBlock_t& block);
};

std::ostream& operator<<(std::ostream& os, const BasicBlock_t& block);

