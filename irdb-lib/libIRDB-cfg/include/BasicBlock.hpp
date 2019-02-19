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


namespace libIRDB
{
	using namespace std;

	class BasicBlock_t : public IRDB_SDK::BasicBlock_t
	{

		public:
			BasicBlock_t();
			virtual ~BasicBlock_t() { }

			bool getIsExitBlock() const { return is_exit_block; }
			void setIsExitBlock(bool is_exit) { is_exit_block=is_exit; }

			IRDB_SDK::InstructionVector_t& GetInstructions() { return instructions; }
			IRDB_SDK::BasicBlockSet_t&     GetPredecessors() { return predecessors; }
			IRDB_SDK::BasicBlockSet_t&     GetSuccessors()   { return successors; }
			IRDB_SDK::BasicBlockSet_t&     GetIndirectTargets() { return indirect_targets; }

			// for const correctness if you aren't modifying.
			const IRDB_SDK::InstructionVector_t& getInstructions()    const { return instructions; }
			const IRDB_SDK::BasicBlockSet_t&     getPredecessors()    const { return predecessors; }
			const IRDB_SDK::BasicBlockSet_t&     getSuccessors()      const { return successors; }
			const IRDB_SDK::BasicBlockSet_t&     getIndirectTargets() const { return indirect_targets; }
//			      IRDB_SDK::BasicBlock_t   *     getFallthrough()     const ;
//			      IRDB_SDK::BasicBlock_t   *     getTarget()          const ;

			bool endsInBranch() const;
			bool endsInIndirectBranch() const;
			bool endsInConditionalBranch() const;
			IRDB_SDK::Instruction_t* getBranchInstruction() const;
			void dump(ostream &os=cout) const ; 

		protected:

			void BuildBlock( IRDB_SDK::Instruction_t* insn,
					const map<IRDB_SDK::Instruction_t*,BasicBlock_t*> &insn2block_map
					);


		private:

			IRDB_SDK::InstructionVector_t instructions;
			IRDB_SDK::BasicBlockSet_t  predecessors;
			IRDB_SDK::BasicBlockSet_t  successors;
			IRDB_SDK::BasicBlockSet_t  indirect_targets;
			bool is_exit_block;

		friend ostream& operator<<(ostream& os, const BasicBlock_t& block);
		friend class ControlFlowGraph_t;
	};


}
