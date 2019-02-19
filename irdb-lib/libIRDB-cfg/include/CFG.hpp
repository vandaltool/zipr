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

	class ControlFlowGraph_t : public IRDB_SDK::ControlFlowGraph_t
	{
		public:
			ControlFlowGraph_t(IRDB_SDK::Function_t* func);
			virtual ~ControlFlowGraph_t() { }
			IRDB_SDK::BasicBlock_t* getEntry() const { return entry; }
			IRDB_SDK::Function_t* getFunction() const { return function; }
			IRDB_SDK::BasicBlockSet_t& GetBlocks()   { return blocks; }
			const IRDB_SDK::BasicBlockSet_t& getBlocks()   const { return blocks; }
			void dump(ostream &os=cout) const; 
			bool hasEdge(IRDB_SDK::BasicBlock_t *p_src, IRDB_SDK::BasicBlock_t *p_tgt) const;
			IRDB_SDK::CFGEdgeType_t getEdgeType(const IRDB_SDK::BasicBlock_t *p_src, const IRDB_SDK::BasicBlock_t *p_tgt) const;

		private:
		// methods 
			void Build(IRDB_SDK::Function_t *func);
			void alloc_blocks(const IRDB_SDK::InstructionSet_t &starts, map<IRDB_SDK::Instruction_t*,BasicBlock_t*>& insn2block_map);
			void build_blocks(const map<IRDB_SDK::Instruction_t*,BasicBlock_t*>& insn2block_map);
			void find_unblocked_instructions(InstructionSet_t &starts, IRDB_SDK::Function_t* func);

		// data
			IRDB_SDK::BasicBlockSet_t blocks;
			IRDB_SDK::BasicBlock_t* entry;
			IRDB_SDK::Function_t* function;

	};

}
