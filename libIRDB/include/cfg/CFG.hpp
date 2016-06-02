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



typedef std::set<BasicBlock_t*> BasicBlockSet_t;

class ControlFlowGraph_t
{
	public:
		ControlFlowGraph_t(Function_t* func);

		BasicBlock_t* GetEntry() const { return entry; }
		Function_t* GetFunction() const { return function; }

	protected:
		void Build(Function_t *func);

	private:
		std::set<BasicBlock_t*> blocks;
		BasicBlock_t* entry;
		Function_t* function;

	/* friends */
	public:
		friend std::ostream& operator<<(std::ostream& os, const ControlFlowGraph_t& cfg);
		BasicBlockSet_t& GetBlocks()   { return blocks; }
		const BasicBlockSet_t& GetBlocks()   const { return blocks; }
};


std::ostream& operator<<(std::ostream& os, const ControlFlowGraph_t& cfg);


