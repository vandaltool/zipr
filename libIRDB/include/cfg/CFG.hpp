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

enum CFG_EdgeTypeEnum { CFG_FallthroughEdge, CFG_TargetEdge, CFG_IndirectEdge };
typedef std::set<CFG_EdgeTypeEnum> CFG_EdgeType;

class ControlFlowGraph_t
{
	public:
		ControlFlowGraph_t(IRDB_SDK::Function_t* func);
		BasicBlock_t* GetEntry() const { return entry; }
		IRDB_SDK::Function_t* getFunction() const { return function; }
		BasicBlockSet_t& GetBlocks()   { return blocks; }
		const BasicBlockSet_t& GetBlocks()   const { return blocks; }
		void dump(std::ostream &os=std::cout) const { os<<*this; }
		bool HasEdge(BasicBlock_t *p_src, BasicBlock_t *p_tgt) const;
		CFG_EdgeType GetEdgeType(const BasicBlock_t *p_src, const BasicBlock_t *p_tgt) const;

	private:
	// methods 
		void Build(IRDB_SDK::Function_t *func);
		void alloc_blocks(const IRDB_SDK::InstructionSet_t &starts, map<IRDB_SDK::Instruction_t*,BasicBlock_t*>& insn2block_map);
		void build_blocks(const map<IRDB_SDK::Instruction_t*,BasicBlock_t*>& insn2block_map);
		void find_unblocked_instructions(InstructionSet_t &starts, IRDB_SDK::Function_t* func);

	// data
		BasicBlockSet_t blocks;
		BasicBlock_t* entry;
		IRDB_SDK::Function_t* function;

	/* friends */
	public:
		friend std::ostream& operator<<(std::ostream& os, const ControlFlowGraph_t& cfg);
};


std::ostream& operator<<(std::ostream& os, const ControlFlowGraph_t& cfg);


