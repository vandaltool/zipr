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


#include <stdlib.h>
#include <cmath>

#include "utils.hpp"
#include "scdi_instr.hpp"
#include "Rewrite_Utility.hpp"
//#include <bea_deprecated.hpp>

using namespace std;
using namespace libIRDB;


template< typename T >
std::string int_to_hex_string( T i )
{
  std::stringstream stream;
  stream << "0x" 
         << std::hex << i;
  return stream.str();
}

bool SimpleCDI_Instrument::add_scdi_instrumentation(Instruction_t* insn)
{
	bool success=true;

	if(getenv("SimpleCDI_VERBOSE")!=NULL)
	{
		cout<<"Found that "<<insn->GetBaseID()<<":"<<insn->getDisassembly()<<" can be converted to CDI"<<endl;
	}

	ICFS_t* ibts=insn->GetIBTargets();
	//DISASM d;
	//Disassemble(insn,d);
	const auto d=DecodedInstruction_t(insn);

	if(getenv("SimpleCDI_VERBOSE")!=NULL && ibts)
	{
		cout <<"["<<d.getDisassembly()<<"] [" << d.getMnemonic()<< "] IBTargets size: " << ibts->size() << " analysis_status: " << ibts->GetAnalysisStatus() << endl;
	}

	if (is_return(insn))
	{
		// instrumentation must be coordinated with needs_scdi_instrumentation()
		if (ibts && ibts->IsComplete() && ibts->size() == 1)
		{
			Instruction_t *return_site = NULL;
			for(ICFS_t::iterator it=ibts->begin(); it!=ibts->end(); ++it)
			{
				return_site=*it; 
			}

			Instruction_t *ret;
			if (firp->GetArchitectureBitWidth() == 64)
				ret = insertAssemblyBefore(firp,insn,"lea rsp, [rsp+8]");
			else if (firp->GetArchitectureBitWidth() == 32)
				ret = insertAssemblyBefore(firp,insn,"lea esp, [esp+4]");
			else
				assert(0);
	
			ret->Assemble("jmp 0");
			ret->SetFallthrough(NULL);
			ret->SetTarget(return_site);
			ret->SetIBTargets(NULL);

			cout<<hex<<insn->GetAddress()->GetVirtualOffset();
			cout<<": Converted ret into a direct jmp: "<<insn->getDisassembly();
			cout<<"   jmp back to: "<<ret->GetTarget()->GetAddress()->GetVirtualOffset()<<": "<<ret->GetTarget()->getDisassembly()<<dec<<endl;
			single_target_set_returns++;
			return true;
		}
	}

	//assert(strstr("ret ", d.Instruction.Mnemonic)==NULL);
	//assert(strstr("retn ", d.Instruction.Mnemonic)==NULL);
	assert(!d.isReturn()) ;
	
	// pre-instrument
	// push reg
	// mov reg, <target>
	string reg="rcx";
	//string addr_mode=(strstr(d.CompleteInstr," "));
	string addr_mode=d.getOperand(0).getString();

	Instruction_t* tmp=insn;
	insertAssemblyBefore(firp,tmp,"push "+reg);
	tmp=insertAssemblyAfter(firp,tmp,"mov "+reg+", "+addr_mode);

	for(ICFS_t::iterator it=ibts->begin(); it!=ibts->end(); ++it)
	{
		Instruction_t* target=*it; 
		// add:  
		//	<t> pop reg  -> fallthrough to <target>
		// insert before:
		//	cmp reg, <target>; 
		//	je <t>
		assert(target && target->GetIndirectBranchTargetAddress() 
			&& target->GetIndirectBranchTargetAddress()->GetVirtualOffset());
	
		if(getenv("SimpleCDI_VERBOSE")!=NULL)
			cout<<"Adding check for "<<hex<<target->GetIndirectBranchTargetAddress()->GetVirtualOffset()<<endl;

		Instruction_t *t=addNewAssembly(firp,NULL, string("pop ")+reg);
		t->SetFallthrough(target);

		tmp=insertAssemblyAfter(firp,tmp, "cmp "+reg+", "+ 
			int_to_hex_string(target->GetIndirectBranchTargetAddress()->GetVirtualOffset()));
		tmp=insertAssemblyAfter(firp,tmp,"je 0x0",t); 
	}

	// add hlt instrution and/or controlled exit callback.
	tmp=insertAssemblyAfter(firp,tmp,"hlt"); 
	
	// leave original instruction, because i'm lazy.
	return success;
}

bool SimpleCDI_Instrument::is_return(Instruction_t* insn)
{
	if (insn) 
	{
		//DISASM d;
		//Disassemble(insn,d);
		const auto d=DecodedInstruction_t(insn);
		return d.isReturn(); // string(d.Instruction.Mnemonic) == string("ret "); 

		// FIXME: handle retn immd, but this means the instrumentation should pop/lea immd
	/*	return (string(d.Instruction.Mnemonic) == string("ret ") ||
		    string(d.Instruction.Mnemonic) == string("retn "));
	*/
	}

	return false;
}

// only complete returns need to be instrumented
bool SimpleCDI_Instrument::needs_scdi_instrumentation(Instruction_t* insn, uint32_t target_size_threshold)
{
	const bool isReturn = is_return(insn);

	if (isReturn)
		num_returns++;

	ICFS_t* ibts=insn->GetIBTargets();
	if(!ibts)
		return false;

	if (ibts->IsComplete() && ibts->size() > 0)
	{
		num_complete_ibts++;
		if (isReturn)
			num_complete_returns++;
	}

	if (isReturn)
	{
		if (ibts->IsComplete())
		{
			if (target_set_threshold < 0)
				return true;
			else 
				return ibts->size() <= target_size_threshold;
		}
		else 
			return false;
	}

	return false;
}

bool SimpleCDI_Instrument::convert_ibs()
{
	bool success=true;

        // we do this in two passes.  first pass:  find instructions.
        for(InstructionSet_t::iterator it=firp->GetInstructions().begin();
                it!=firp->GetInstructions().end();
                ++it)
        {
		Instruction_t* insn=*it;
		if(needs_scdi_instrumentation(insn, target_set_threshold))
			success = success && add_scdi_instrumentation(insn);
	}

	return success;
}

void SimpleCDI_Instrument::display_stats(std::ostream &out)
{
	float fraction = NAN;
	out << "# ATTRIBUTE target_set_threshold=" << dec << target_set_threshold << endl;
	out << "# ATTRIBUTE complete_ibts=" << dec << num_complete_ibts << endl;
	out << "# ATTRIBUTE num_returns=" << num_returns << endl;
	if (num_complete_returns>0)
		fraction = (float)num_complete_returns/num_returns;
	out << "# ATTRIBUTE num_complete_returns=" << num_complete_returns << endl;
	out << "# ATTRIBUTE complete_returns_fraction=" << fraction << endl;
	out << "# ATTRIBUTE single_target_set_jumps=" << single_target_set_jumps << endl;
	out << "# ATTRIBUTE single_target_set_returns=" << single_target_set_returns << endl;

	fraction = NAN;
	if (num_complete_ibts > 0)
		fraction = (float)(single_target_set_returns)/num_returns;
	out << "# ATTRIBUTE single_target_set_return_fraction=" << fraction << endl;
}

/* CDI: control data isolation */
bool SimpleCDI_Instrument::execute()
{

	bool success=true;

	success = success && convert_ibs();

	display_stats(cout);

	return success;
}


