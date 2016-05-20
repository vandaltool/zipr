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


#include <map>
#include <libIRDB-core.hpp>
#include <libIRDB-util.hpp>
#include <utils.hpp>

using namespace libIRDB;
using namespace std;



void InstructionPredecessors_t::AddPreds(const Instruction_t* before, const InstructionSet_t& afterset)
{
	for(InstructionSet_t::const_iterator it=afterset.begin(); it!=afterset.end(); ++it)
	{
		pred_map[*it].insert((libIRDB::Instruction_t*)before);
	}
}

void InstructionPredecessors_t::AddPred(const Instruction_t* before, const Instruction_t* after)
{
	assert(before);
	if(!after) return;

	if(getenv("DUMP_PRED_CREATE"))
		cout<<"Found "<<after->GetBaseID()<<":"<<after->getDisassembly() << " follows "<< before->GetBaseID()<<":"<<before->getDisassembly()<<endl;
	pred_map[after].insert((Instruction_t*)before);
	
}

void InstructionPredecessors_t::AddFile(const FileIR_t* firp2)
{
	FileIR_t* firp=(FileIR_t*)firp2; // discarding const qualifier because we know we won't change the set
	for(InstructionSet_t::const_iterator it=firp->GetInstructions().begin();
		it!=firp->GetInstructions().end();
		++it)
	{
		Instruction_t* insn=*it;
		AddPred(insn, insn->GetTarget());
		AddPred(insn, insn->GetFallthrough());

		if(insn->GetIBTargets())
			AddPreds(insn, *insn->GetIBTargets());
		
	}
}
