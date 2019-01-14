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


class ControlFlowGraph_t
{
	public:
		ControlFlowGraph_t(Function_t* func);
		BasicBlock_t* GetEntry() const { return entry; }
		Function_t* GetFunction() const { return function; }
		BasicBlockSet_t& GetBlocks()   { return blocks; }
		const BasicBlockSet_t& GetBlocks()   const { return blocks; }
		void dump(std::ostream &os=std::cout) const { os<<*this; }

	private:
	// methods 
		void Build(Function_t *func);
		void alloc_blocks(const InstructionSet_t &starts, map<Instruction_t*,BasicBlock_t*>& insn2block_map);
		void build_blocks(const map<Instruction_t*,BasicBlock_t*>& insn2block_map);
		void find_unblocked_instructions(InstructionSet_t &starts, Function_t* func);

	// data
		BasicBlockSet_t blocks;
		BasicBlock_t* entry;
		Function_t* function;

	/* friends */
	public:
		friend std::ostream& operator<<(std::ostream& os, const ControlFlowGraph_t& cfg);
};


std::ostream& operator<<(std::ostream& os, const ControlFlowGraph_t& cfg);


