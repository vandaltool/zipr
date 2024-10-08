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
#include <irdb-util>

using namespace libIRDB;
using namespace std;



void InstructionPredecessors_t::AddPreds(const IRDB_SDK::Instruction_t* before, const InstructionSet_t& afterset)
{
	for(auto it=afterset.begin(); it!=afterset.end(); ++it)
	{
		pred_map[*it].insert(const_cast<IRDB_SDK::Instruction_t*>(before));
	}
}

void InstructionPredecessors_t::AddPred(const IRDB_SDK::Instruction_t* before, const IRDB_SDK::Instruction_t* after)
{
	assert(before);
	if(!after) return;

	if(getenv("DUMP_PRED_CREATE"))
		cout<<"Found "<<after->getBaseID()<<":"<<after->getDisassembly() << " follows "<< before->getBaseID()<<":"<<before->getDisassembly()<<endl;
	pred_map[after].insert(const_cast<IRDB_SDK::Instruction_t*>(before));
	
}

void InstructionPredecessors_t::AddFile(const IRDB_SDK::FileIR_t* firp2)
{
	auto firp=const_cast<IRDB_SDK::FileIR_t*>(firp2); // discarding const qualifier because we know we won't change the set
	for(auto it=firp->getInstructions().begin();
		it!=firp->getInstructions().end();
		++it)
	{
		auto insn=*it;
		AddPred(insn, insn->getTarget());
		AddPred(insn, insn->getFallthrough());

		// if we have a complete list, then explicitly add them.
	        if(insn->getIBTargets() && insn->getIBTargets()->isComplete())
        	      	AddPreds(insn, *insn->getIBTargets());
		
	}
}

unique_ptr<IRDB_SDK::InstructionPredecessors_t> IRDB_SDK::InstructionPredecessors_t::factory(IRDB_SDK::FileIR_t* firp)
{
	return unique_ptr<IRDB_SDK::InstructionPredecessors_t>(new libIRDB::InstructionPredecessors_t(firp));
}

