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

#include <all.hpp>
#include <core/ibtargets.hpp>
#include <utils.hpp>

using namespace std;

IBTargets::IBTargets() {}

IBTargets::~IBTargets()
{
	for(ICFGNodeMap_t::const_iterator i=m_ibtMap.begin(); i!=m_ibtMap.end(); ++i)
	{
		InstructionCFGNode_t *node = i->second;
		delete node;
	}

	std::map<ICFGHellnodeType, InstructionCFGNode_t*>::const_iterator it;
	for(it=m_hellnodeMap.begin(); it!=m_hellnodeMap.end(); ++it)
	{
		InstructionCFGNode_t *node = it->second;
		delete node;
	}
}

void IBTargets::AddTarget(Instruction_t* const instr, Instruction_t* const ibtarget)
{
	assert(instr && ibtarget);

	InstructionCFGNode_t *ibtargetnode = m_ibtMap[ibtarget];
	if (!ibtargetnode)
	{
		ibtargetnode = new InstructionCFGNode_t(ibtarget);
		m_ibtMap[ibtarget] = ibtargetnode;
	}

	// update internal map
	m_ibtargets[instr].insert(ibtargetnode);

	// update instruction ib targets
	instr->GetIBTargets().insert(ibtargetnode);
}

void IBTargets::RemoveTarget(Instruction_t* const instr, InstructionCFGNode_t* const node)
{
	if (m_ibtargets.find(instr) != m_ibtargets.end())
	{
		m_ibtargets[instr].erase(node); 
		instr->GetIBTargets().erase(node);
	}
}

void IBTargets::Remove(Instruction_t* const instr)
{
	if (m_ibtargets.find(instr) != m_ibtargets.end())
	{
		m_ibtargets.erase(instr);
		instr->GetIBTargets().clear();

		// need to mark entry so that we can clear from DB
		// but need to make sure we don't clear if there's an entry
	}
}

void IBTargets::AddHellnodeTarget(Instruction_t* const instr, ICFGHellnodeType hntype)
{
	assert(instr);

	InstructionCFGNode_t* hellnode = m_hellnodeMap[hntype];
	if (!hellnode)
	{
		hellnode = new InstructionCFGNode_t(hntype);
		m_hellnodeMap[hntype] = hellnode;
	}

	// update internal map
	m_ibtargets[instr].insert(hellnode);

	// update instruction ib targets
	instr->GetIBTargets().insert(hellnode);
}

void IBTargets::RemoveHellnodeTarget(Instruction_t* const instr, ICFGHellnodeType hntype)
{
	assert(instr);		
	InstructionCFGNode_t* hellnode = m_hellnodeMap[hntype];
	if (hellnode)
	{
		m_ibtargets[instr].erase(hellnode);
		instr->GetIBTargets().erase(hellnode);
	}
}


// @todo: allow finer stepping for striding?
// @todo: need to make sure we erase stale data!!!
// 
string IBTargets::WriteToDB(File_t *fid)
{
	assert(fid);
	
	if (m_ibtMap.size() == 0 && m_hellnodeMap.size() == 0)
		return "";

	string q = "";

	for(IBTargetMap_t::iterator i=m_ibtargets.begin(); i!=m_ibtargets.end(); ++i)
	{
		Instruction_t *instr = i->first;
		InstructionCFGNodeSet_t &nodes = i->second;
		assert(instr);
		for (InstructionCFGNodeSet_t::const_iterator j=nodes.begin(); j!=nodes.end(); ++j)
		{
			InstructionCFGNode_t* node = *j;
			db_id_t instruction_id = instr->GetBaseID();
			db_id_t target_instruction_id = node->IsHellnode() ?
				node->GetHellnodeType() : node->GetInstruction()->GetBaseID();
 			q +=
				string("insert into ") + fid->ibtargets_table_name + 
				string(" (instruction_id,target_instruction_id)") +
				string(" values (") +
				string("'") + to_string(instruction_id) + string("', ") + 
				string("'") + to_string(target_instruction_id) + 
				string("'") + string(");");
		}
	}

//cout << "query: " << q << endl;
	return q;
}

const string IBTargets::toString()
{
	stringstream ss;
	std::map<ICFGHellnodeType, InstructionCFGNode_t*>::iterator hit;
	ss << "Hellnode Entries:" << endl;
	for (hit = m_hellnodeMap.begin(); hit != m_hellnodeMap.end(); ++hit)
	{
		ICFGHellnodeType hntype = hit->first;
		ss << "  hellnode type: " << hntype << endl;
	}

	ss << endl;
	ss << "IB Targets Map:" << endl;
	IBTargetMap_t::iterator it;
	for (it = m_ibtargets.begin(); it != m_ibtargets.end(); ++it)
	{
		Instruction_t* insn = it->first;
		InstructionCFGNodeSet_t nodeset = it->second;

		ss << "  Instr: " << hex << insn->GetAddress()->GetVirtualOffset() << " [" << insn->getDisassembly() << "] --> ";

		InstructionCFGNodeSet_t::iterator j;
		for (j = nodeset.begin(); j != nodeset.end(); ++j)
		{
			InstructionCFGNode_t* node = *j;
			if (node->IsHellnode())
			{
				ss << "hellnode(" << dec << node->GetHellnodeType() << ") ";
			}
			else
			{
				ss << node->GetInstruction()->GetAddress()->GetVirtualOffset() << " ";
			}
		}

		ss << endl;
	}

	return ss.str();
}
