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
#include <irdb-util>
#include <algorithm>

using namespace libIRDB;
using namespace std;

// Does instruction potentially write to a parameter to a call?
bool IRDB_SDK::isParameterWrite(const IRDB_SDK::FileIR_t *firp, IRDB_SDK::Instruction_t* insn, string& output_dst)
{
	const auto p_d=DecodedInstruction_t::factory(insn);
	const auto &d=*p_d;

	if(!d.hasOperand(0))
		return false;
	if(!d.getOperand(0)->isWritten())
		return false;

	/* 64 bit machines use regs to pass parameters */
	if(firp->getArchitectureBitWidth()==64)
	{
		// if it's a register
		if(d.getOperand(0)->isGeneralPurposeRegister())
		{
			int regno=d.getOperand(0)->getRegNumber();
			switch(regno)
			{
				case 1:  // rcx?
				case 2:  // rdx?
				case 6:  // rsi?
				case 7:  // rdi?
				case 8:  // r8?
				case 9:  // r9?
					output_dst=d.getOperand(0)->getString();
					return true;

				// other regsiters == no.
				default:
					return false;
			}

		}
	}

	// not a register or not 64-bit.  check for [esp+k]

	// check for memory type
	if(!d.getOperand(0)->isMemory())
		return false;

	// check that base reg is esp.
	if(!d.getOperand(0)->hasBaseRegister())
		return false;
	if(d.getOperand(0)->getBaseRegister() != 4)
		return false;

	// check that there's no index reg
	if(d.getOperand(0)->hasIndexRegister())
		return false;

	// get k out of [esp + k ]
	if (!d.getOperand(0)->hasMemoryDisplacement())
		return false;
	const auto k=d.getOperand(0)->getMemoryDisplacement();

	// check that we know the frame layout.
	if(insn->getFunction() == NULL)
		return false;

	if(k < insn->getFunction()->getOutArgsRegionSize())
	{
		output_dst=string("[")+d.getOperand(0)->getString()+string("]");
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
IRDB_SDK::Instruction_t* isOrWasCall(const IRDB_SDK::FileIR_t *firp, IRDB_SDK::Instruction_t* insn)
{
	if (firp == NULL || insn == NULL)
		return NULL;

	const auto p_d=DecodedInstruction_t::factory(insn);
	const auto &d=*p_d;

	if(d.isCall())
	{
		return insn->getTarget();
	}
	else 
	{
		// look for "push64" or "fix_call_fallthrough" reloc
		auto it = std::find_if(insn->getRelocations().begin(),insn->getRelocations().end(),[&](const IRDB_SDK::Relocation_t* reloc) 
		{
			return (reloc && ((reloc->getType() == string("push64")) || reloc->getType() == string("fix_call_fallthrough")));
		});

		// find actual target
		if (it != insn->getRelocations().end())
		{
			if (insn->getTarget())
				return insn->getTarget();
			else if (insn->getFallthrough())
				return insn->getFallthrough()->getTarget();
		}
	}

	return NULL;
}

// Does a call follow the instruction?
bool IRDB_SDK::callFollows(const IRDB_SDK::FileIR_t *firp, IRDB_SDK::Instruction_t* insn, const string& reg_arg_str, const std::string &fn_pattern)
{
	const std::set<std::string> param_regs = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
	const bool original_is_param_register = param_regs.find(reg_arg_str) != param_regs.end();

	std::set<std::string> live_params;
	live_params.insert(reg_arg_str);
	
	for(auto ptr=insn->getFallthrough(); ptr!=NULL; ptr=ptr->getFallthrough())
	{
		const auto p_d=DecodedInstruction_t::factory(ptr);
		const auto &d=*p_d;
		const auto tgt = isOrWasCall(firp, ptr);

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
				if(tgt->getFunction()==NULL) 
					return false;

				const auto fname = tgt->getFunction()->getName();
//				std::cout << "CallFollows(): yes, parameter to call: function name: " << fname << endl;
				return fname.find(fn_pattern)!=string::npos;
			}
		}

		// found reference to argstring, original code would just stop the search
		// need more sophisticated heuristic
		if(d.getDisassembly().find(reg_arg_str)!= string::npos)
		{
			if (original_is_param_register)
			{
				std::string arg;
				if (isParameterWrite(firp, ptr, arg))
				{
//					std::cout << "CallFollows(): " << ptr->getDisassembly() << ": detected write parameter" << std::endl;
 					if (arg == reg_arg_str)
					{
//						std::cout << "CallFollows(): " << ptr->getDisassembly() << ": same parameter as original: remove from live list: " << reg_arg_str << std::endl;
						live_params.erase(reg_arg_str);
					}
					else 
					{
						if (d.hasOperand(1) && d.getOperand(1)->getString() == reg_arg_str) 
						{
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

bool IRDB_SDK::flowsIntoCall(const IRDB_SDK::FileIR_t *firp, IRDB_SDK::Instruction_t* insn)
{
	string param_write;
	if (!isParameterWrite(firp, insn, param_write))
		return false;

	return callFollows(firp, insn, param_write);
}

bool IRDB_SDK::leaFlowsIntoCall(const IRDB_SDK::FileIR_t *firp, IRDB_SDK::Instruction_t* insn)
{
	const auto d=DecodedInstruction_t::factory(insn);

	if (d->getMnemonic()!="lea")
		return false;

	return flowsIntoCall(firp, insn);
}

bool IRDB_SDK::leaFlowsIntoPrintf(const IRDB_SDK::FileIR_t *firp, IRDB_SDK::Instruction_t* insn)
{
	const auto d=DecodedInstruction_t::factory(insn);

	if (d->getMnemonic()!="lea")
		return false;

//	std::cout << "LeaFlowsIntoCall(): investigating " << insn->getDisassembly() << endl;

	string param_write;
	if (!isParameterWrite(firp, insn, param_write))
		return false;

	return callFollows(firp, insn, param_write, "printf");
}
