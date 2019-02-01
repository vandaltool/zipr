/*
 * Copyright (c) 2014 - Zephyr Software
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

#include <irdb-core>
#include <irdb-util>
#include <irdb-syscall>
#include <libIRDB-syscall.hpp>
#include <stdlib.h>

using namespace std;
using namespace libIRDB;

Syscalls_t::~Syscalls_t()
{
	for(auto site : syscalls)
		delete site;
	syscalls.clear();
}

IRDB_SDK::SyscallNumber_t Syscalls_t::FindSystemCallNumber(IRDB_SDK::Instruction_t* insn, const IRDB_SDK::InstructionPredecessors_t& preds)
{
	IRDB_SDK::Instruction_t *pred_insn=nullptr;

	for( auto cs_preds=&preds[insn];
		cs_preds->size()==1;
		cs_preds=&preds[pred_insn]
	   )
	{
		pred_insn=*(cs_preds->begin());

		string disass=pred_insn->getDisassembly();

		/* look for eax being used or set */
		if(disass.find("eax")!=string::npos)
		{
			string to_find="mov eax,";
			size_t loc=disass.find(to_find);
			// check to see if it's a mov of eax
			if(loc ==string::npos)
			{
				// no, quit looking backwards 
				return IRDB_SDK::sntUnknown;
			}
			size_t val=strtol(disass.substr(loc+to_find.length()).c_str(),nullptr,16);
			// check to see if it's a mov of eax

			return (IRDB_SDK::SyscallNumber_t)val;
		}
	}

	return IRDB_SDK::sntUnknown;

}

bool Syscalls_t::FindSystemCalls(const IRDB_SDK::FileIR_t *firp2)
{
	auto firp=const_cast<IRDB_SDK::FileIR_t*>(firp2); // discard const qualifer, but we aren't changing anything.
	assert(firp);
	auto predsp=IRDB_SDK::InstructionPredecessors_t::factory(firp);
	auto &preds = * predsp;

	bool found_one=false;
	for(auto insn : firp->getInstructions())
	{
		if(insn->getDisassembly()=="int 0x80") 
		{
			auto num=FindSystemCallNumber(insn, preds);
			syscalls.insert(new libIRDB::SyscallSite_t(insn,num));
			found_one=true;

			if(getenv("PRINTSYSCALLSFOUND"))
				cout<<"Found system call "<< insn->getBaseID()<< ":'"<< insn->getDisassembly() << "' with eax value "<< (size_t)num<<endl;
		}
	}
	return found_one;

}



