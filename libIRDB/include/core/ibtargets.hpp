/*
 * Copyright (c) 2014-2015 - Zephyr Software LLC
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

DEPRECATED DO NOT USE
#ifndef _ibtargets_
#define _ibtargets_

typedef std::map<Instruction_t*, InstructionCFGNodeSet_t> IBTargetMap_t;
typedef	std::map<Instruction_t*, InstructionCFGNode_t*> ICFGNodeMap_t;

// keep track of ib targets
class IBTargets {
	public:
		IBTargets();
		~IBTargets();

		void AddTarget(Instruction_t* const instr, Instruction_t* const ibtarget);
		void RemoveTarget(Instruction_t* const instr, InstructionCFGNode_t* const);
		void AddHellnodeTarget(Instruction_t* const instr, ICFGHellnodeType = DEFAULT_ICFG_HELLNODE);
		void RemoveHellnodeTarget(Instruction_t* const instr, ICFGHellnodeType = DEFAULT_ICFG_HELLNODE);

		void Remove(Instruction_t* const instr);

		std::string WriteToDB(File_t *fid);
		const std::string toString();

	private:
		IBTargetMap_t m_ibtargets;  

		// keep track of allocated nodes
		std::map<ICFGHellnodeType, InstructionCFGNode_t*> m_hellnodeMap;
		ICFGNodeMap_t m_ibtMap;
};

#endif
