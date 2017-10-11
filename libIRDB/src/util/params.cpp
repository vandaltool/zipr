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
	auto d=DISASM({0});
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

//
// we look for:
//    (1) call instruction
//    (2) call converted to push/jmp pair
//
static Instruction_t* IsOrWasCall(const FileIR_t *firp, Instruction_t* insn)
{
	if (firp == NULL || insn == NULL)
		return NULL;

	auto d=DISASM({0});
	insn->Disassemble(d);
	if(d.Instruction.Mnemonic == string("call "))
	{
		return insn->GetTarget();
	}
	else 
	{
		// look for "push64" or "fix_call_fallthrough" reloc
		auto it = std::find_if(insn->GetRelocations().begin(),insn->GetRelocations().end(),[&](const Relocation_t* reloc) 
		{
			return (reloc && ((reloc->GetType() == string("push64")) || reloc->GetType() == string("fix_call_fallthrough")));
		});

		// find actual target
		if (it != insn->GetRelocations().end())
		{
			if (insn->GetTarget())
				return insn->GetTarget();
			else if (insn->GetFallthrough())
				return insn->GetFallthrough()->GetTarget();
		}
	}

	return false;
}

// Does a call follow the instruction?
bool libIRDB::CallFollows(const FileIR_t *firp, Instruction_t* insn, const string& reg_arg_str, const std::string &fn_pattern)
{
	const std::set<std::string> param_regs = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
	const bool original_is_param_register = param_regs.find(reg_arg_str) != param_regs.end();

	std::set<std::string> live_params;
	live_params.insert(reg_arg_str);
	
	for(Instruction_t* ptr=insn->GetFallthrough(); ptr!=NULL; ptr=ptr->GetFallthrough())
	{
		auto d=DISASM({0});
		ptr->Disassemble(d);
		long long vo = 0;
		if (ptr->GetAddress())
			vo = ptr->GetAddress()->GetVirtualOffset();
//		std::cout << "CallFollows(): " << ptr->getDisassembly() << " @ " << hex << vo << std::endl;

		const auto tgt = IsOrWasCall(firp, ptr);

		if (tgt) 
		{
			if (fn_pattern.size() == 0)
			{
				// don't care about function name
//				std::cout << "CallFollows(): yes, parameter to call" << endl;
				return true;
			}
			else 
			{
				// look for specific function

				// check the target has a function 
				if(tgt->GetFunction()==NULL) 
					return false;

				const auto fname = tgt->GetFunction()->GetName();
//				std::cout << "CallFollows(): yes, parameter to call: function name: " << fname << endl;
				return fname.find(fn_pattern)!=string::npos;
			}
		}

		// found reference to argstring, original code would just stop the search
		// need more sophisticated heuristic
		if(string(d.CompleteInstr).find(reg_arg_str)!= string::npos)
		{
			if (original_is_param_register)
			{
				std::string arg;
				if (IsParameterWrite(firp, ptr, arg))
				{
//					std::cout << "CallFollows(): " << ptr->getDisassembly() << ": detected write parameter" << std::endl;
 					if (arg == reg_arg_str)
					{
//						std::cout << "CallFollows(): " << ptr->getDisassembly() << ": same parameter as original: remove from live list: " << reg_arg_str << std::endl;
						live_params.erase(reg_arg_str);
					}
					else 
					{
						if (std::string(d.Argument2.ArgMnemonic) == reg_arg_str) {
//							std::cout << "CallFollows(): " << ptr->getDisassembly() << ": copy of original detected: add to live list: " << arg << std::endl;
							live_params.insert(arg);
						}
					}
				}

				std::cout << "CallFollows(): " << ptr->getDisassembly() << ": #live_param: " << dec << live_params.size() << std::endl;
				if (live_params.size() == 0)
				{
//					std::cout << "CallFollows(): not a parameter to call" << endl;
					return false;
				}
			}
			else
			{
//				std::cout << "\tassume it's a write and exit" << std::endl;
				return false;
			}

		}
	}

//	std::cout << "CallFollows(): no more fallthroughs, not a parameter to call" << endl;
	return false;
}

bool libIRDB::FlowsIntoCall(const FileIR_t *firp, Instruction_t* insn)
{
	auto d=DISASM({0});
	insn->Disassemble(d);

	string param_write;
	if (!libIRDB::IsParameterWrite(firp, insn, param_write))
		return false;

	return CallFollows(firp, insn, param_write);
}

bool libIRDB::LeaFlowsIntoCall(const FileIR_t *firp, Instruction_t* insn)
{
	auto d=DISASM({0});
	insn->Disassemble(d);

	if(string(d.Instruction.Mnemonic)!="lea ")
		return false;

	return FlowsIntoCall(firp, insn);
}

bool libIRDB::LeaFlowsIntoPrintf(const FileIR_t *firp, Instruction_t* insn)
{
	auto d=DISASM({0});
	insn->Disassemble(d);

	if(string(d.Instruction.Mnemonic)!="lea ")
		return false;

//	std::cout << "LeaFlowsIntoCall(): investigating " << insn->getDisassembly() << endl;

	string param_write;
	if (!libIRDB::IsParameterWrite(firp, insn, param_write))
		return false;

	return CallFollows(firp, insn, param_write, "printf");
}
