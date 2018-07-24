/*
 * Copyright (c) 2014, 2015 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

#include "fix_rets.hpp"

#include <assert.h>

using namespace libTransform;

FixRets::FixRets(FileIR_t *p_variantIR) : Transform(NULL, p_variantIR, NULL) 
{
	
}

int FixRets::execute()
{
	string register_stack_pointer;
	string stack_offset_size;
	string pop_insn_assembly, ret_insn_assembly;

	if (getFileIR()->GetArchitectureBitWidth() == 64)
	{
		register_stack_pointer = "rsp";
		stack_offset_size = "8";
	}
	else
	{
		register_stack_pointer = "esp";
		stack_offset_size = "4";
	}
	pop_insn_assembly = "lea " + register_stack_pointer +
		", [" + register_stack_pointer + 
		"+" + stack_offset_size + "]";
	ret_insn_assembly = "jmp [" + 
		register_stack_pointer + 
		"-" + stack_offset_size + "]";	
	cout << "pop_insn_assembly: " << pop_insn_assembly << endl;
	cout << "ret_insn_assembly: " << ret_insn_assembly << endl;

	for(
	  set<Function_t*>::const_iterator itf=getFileIR()->GetFunctions().begin();
	  itf!=getFileIR()->GetFunctions().end();
	  ++itf
	  )
	{
		Function_t* func=*itf;
		for(
		  set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
		  it!=func->GetInstructions().end();
		  ++it)
		{
			Instruction_t* insn = *it;
			if(insn&& insn->GetAddress())
			{
				if (insn!=NULL)
				{
					//DISASM disasm;
					//Disassemble(insn,disasm);
					const auto disasm=DecodedInstruction_t(insn);
					string stack_pointer;
					string stack_offset_size;

					cout << "Complete instruction: " << disasm.getDisassembly() << "-" << endl;
					if (disasm.isReturn()) // strcmp(disasm.CompleteInstr,"ret "))
						continue;

                                        bool isPinned =   insn->GetIndirectBranchTargetAddress()
                                                       && insn->GetIndirectBranchTargetAddress()->GetVirtualOffset()!=0;
					if (isPinned)
					{
						cout << "Skipping ret fix because it's pinned." << endl;
						continue;
					}
					Instruction_t *pop = NULL;
					Instruction_t *ret = insn;
					
					pop = allocateNewInstruction(
						insn->GetAddress()->GetFileID(), insn->GetFunction());
					setAssembly(pop, pop_insn_assembly);
					setAssembly(ret, ret_insn_assembly);

					carefullyInsertBefore(ret, pop);
					pop->SetFallthrough(ret);
					cout << "Fixing a ret!" << endl;
				}
			}
		}
	}
	return true;
}
