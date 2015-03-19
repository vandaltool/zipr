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

#ifndef _instruction_cfg_
#define _instruction_cfg_

// Like Dante, we accomodate multiple layers of hell nodes
typedef enum { DEFAULT_ICFG_HELLNODE = -1 } ICFGHellnodeType;

class InstructionCFGNode_t
{
    public:
		InstructionCFGNode_t(Instruction_t* i) {
			assert(i);
			m_isHellNode = false;					
			m_node.instruction = i;
		}

		InstructionCFGNode_t(ICFGHellnodeType hntype) {
			m_isHellNode = true;
			m_node.hntype = hntype;
		}

		Instruction_t* GetInstruction() const {
			assert(!IsHellnode());					
			return m_node.instruction;
		}

		ICFGHellnodeType GetHellnodeType() const {
			assert(IsHellnode());					
			return m_node.hntype;
		}

		bool IsHellnode() const { return m_isHellNode; }

		bool operator==(const InstructionCFGNode_t &other) {
			if (this == &other) 
				return true;			
			else if (IsHellnode()) 
				return m_node.hntype == other.m_node.hntype;
			else
				return m_node.instruction == other.m_node.instruction;
		}

    private:
		bool m_isHellNode;
		union {
			Instruction_t* instruction;
			ICFGHellnodeType hntype; 
		} m_node; 
};

typedef std::set<InstructionCFGNode_t*> InstructionCFGNodeSet_t;

#endif
