/*
 * Copyright (c) 2014-2017 - Zephyr Software LLC
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


#include <libIRDB-core.hpp>
#include <libIRDB-util.hpp>
#include <utils.hpp>

using namespace libIRDB;
using namespace std;

// Does instruction potentially write to a parameter to a call?
bool libIRDB::IsParameterWrite(const FileIR_t *firp, Instruction_t* insn, string& output_dst)
{
	DISASM d;
	insn->Disassemble(d);
	if(d.Argument1.AccessMode!=WRITE)
	{
		return false;
	}

	/* 64 bit machines use regs to pass parameters */
	if(firp->GetArchitectureBitWidth()==64)
	{
		// if it's a register
		if((d.Argument1.ArgType&REGISTER_TYPE)==REGISTER_TYPE)
		{
			int regno=(d.Argument1.ArgType)&0xFFFF;
			switch(regno)
			{
				case REG7:	// rdi
				case REG6:	// rsi
				case REG2:	// rdx
				case REG1:	// rcx
				case REG8:	// r8
				case REG9:	// r9
					output_dst=d.Argument1.ArgMnemonic;
					return true;

				// other regsiters == no.
				default:
					return false;
			}

		}
	}

	// not a register or not 64-bit.  check for [esp+k]

	// check for memory type
	if((d.Argument1.ArgType&MEMORY_TYPE)!=MEMORY_TYPE)
		return false;

	// check that base reg is esp.
	if(d.Argument1.Memory.BaseRegister != REG4)
		return false;

	// check that there's no index reg
	if(d.Argument1.Memory.IndexRegister != 0)
		return false;

	// get k out of [esp + k ]
	unsigned int k=d.Argument1.Memory.Displacement;

	// check that we know the frame layout.
	if(insn->GetFunction() == NULL)
		return false;

	if(k < insn->GetFunction()->GetOutArgsRegionSize())
	{
		output_dst=string("[")+d.Argument1.ArgMnemonic+string("]");
		return true;
	}

	// return we didn't find a memory of the right type
	return false;
}

// is instruction originally a call?
static bool IsOrWasCall(const FileIR_t *firp, Instruction_t* insn)
{
	if (firp == NULL || insn == NULL)
		return false;

	DISASM d;
	insn->Disassemble(d);
	if(d.Instruction.Mnemonic == string("call "))
		return true;
	else {
		// call may have been converted to push/jmp in previous phase
		// look for "push64" type reloc
		auto it = std::find_if(insn->GetRelocations().begin(),insn->GetRelocations().end(),[&](const Relocation_t* reloc) 
		{
			if (reloc)
				cout << "IsOrWasCall(): reloc: " << reloc->GetType() << endl;
			return (reloc && ((reloc->GetType() == string("push64")) || reloc->GetType() == string("fix_call_fallthrough")));
		});

		if (it != insn->GetRelocations().end())
			return true;
	}

	return false;
}

// Does a call follow the instruction?
bool libIRDB::CallFollows(FileIR_t *firp, Instruction_t* insn, const string& arg_str)
{
	for(Instruction_t* ptr=insn->GetFallthrough(); ptr!=NULL; ptr=ptr->GetFallthrough())
	{
		DISASM d;
		ptr->Disassemble(d);
		if(IsOrWasCall(firp, ptr))
		{
			// found it
			return true;
		}

		// found reference to argstring, assume it's a write and exit
		if(string(d.CompleteInstr).find(arg_str)!= string::npos)
			return false;
	}

	return false;
}

