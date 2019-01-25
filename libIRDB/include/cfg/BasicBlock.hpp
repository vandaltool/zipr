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


class BasicBlock_t;
typedef std::set<BasicBlock_t*> BasicBlockSet_t;
typedef std::vector<IRDB_SDK::Instruction_t*> InstructionVector_t;

class BasicBlock_t
{

	public:
		BasicBlock_t();

		bool GetIsExitBlock() { return is_exit_block; }
		void SetIsExitBlock(bool is_exit) { is_exit_block=is_exit; }

		InstructionVector_t& GetInstructions() { return instructions; }
		BasicBlockSet_t&     GetPredecessors() { return predecessors; }
		BasicBlockSet_t&     GetSuccessors()   { return successors; }
		BasicBlockSet_t&     GetIndirectTargets() { return indirect_targets; }
		BasicBlock_t* GetFallthrough();
		BasicBlock_t* getTarget();

		// for const correctness if you aren't modifying.
		const InstructionVector_t& GetInstructions() const { return instructions; }
		const BasicBlockSet_t&     GetPredecessors() const { return predecessors; }
		const BasicBlockSet_t&     GetSuccessors()   const { return successors; }
		const BasicBlockSet_t&     GetIndirectTargets() const { return indirect_targets; }

		bool EndsInBranch();
		bool EndsInIndirectBranch();
		bool EndsInConditionalBranch();
		IRDB_SDK::Instruction_t* GetBranchInstruction();
		void dump(std::ostream &os=std::cout) const { os<<*this; }

	protected:

		void BuildBlock( IRDB_SDK::Instruction_t* insn,
                		const std::map<IRDB_SDK::Instruction_t*,BasicBlock_t*> &insn2block_map
        			);


	private:

		InstructionVector_t instructions;
		BasicBlockSet_t  predecessors;
		BasicBlockSet_t  successors;
		BasicBlockSet_t  indirect_targets;
		bool is_exit_block;

	friend std::ostream& operator<<(std::ostream& os, const BasicBlock_t& block);
	friend class ControlFlowGraph_t;
};

std::ostream& operator<<(std::ostream& os, const BasicBlock_t& block);

