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

//#include <libIRDB-core.hpp>
//#include <libIRDB-util.hpp>
#include <stdlib.h>
#include <libIRDB-syscall.hpp>

using namespace std;
using namespace libIRDB;

SyscallNumber_t Syscalls_t::FindSystemCallNumber(Instruction_t* insn, const libIRDB::InstructionPredecessors_t& preds)
{
	Instruction_t *pred_insn=NULL;

	for( const InstructionSet_t *cs_preds=&preds[insn];
		cs_preds->size()==1;
		cs_preds=&preds[pred_insn]
	   )
	{
		pred_insn=*(cs_preds->begin());
		// cout<<"Pred is "<<pred_insn->getDisassembly()<<endl;

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
				return SNT_Unknown;
			}
			size_t val=strtol(disass.substr(loc+to_find.length()).c_str(),NULL,16);
			// check to see if it's a mov of eax

			return (SyscallNumber_t)val;
		}
	}

	return SNT_Unknown;

}

bool Syscalls_t::FindSystemCalls(const FileIR_t *firp2)
{
	FileIR_t* firp=(FileIR_t*)firp2; // discard const qualifer, but we aren't changing anything.
	assert(firp);
	libIRDB::InstructionPredecessors_t preds;
	preds.AddFile(firp);

	bool found_one=false;
	for(InstructionSet_t::iterator it=firp->GetInstructions().begin();
			it!=firp->GetInstructions().end();
			++it
	   )
	{
		Instruction_t* insn=*it;
		if(insn->IsSyscall())
		{
			SyscallNumber_t num=FindSystemCallNumber(insn, preds);
			syscalls.insert(SyscallSite_t(insn,num));
			found_one=true;

			if(getenv("PRINTSYSCALLSFOUND"))
				cout<<"Found system call "<< insn->GetBaseID()<< ":'"<< insn->getDisassembly() << "' with eax value "<< (size_t)num<<endl;
		}
	}
	return found_one;

}



