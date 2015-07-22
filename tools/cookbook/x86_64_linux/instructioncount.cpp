#include "instructioncount.hpp"

#include "Rewrite_Utility.hpp"

#include <assert.h>

using namespace libTransform;

InstructionCount::InstructionCount(VariantID_t *p_variantID, FileIR_t *p_variantIR, set<std::string> *p_filteredFunctions) : CookbookTransform(p_variantID, p_variantIR, p_filteredFunctions) 
{
	
}

int InstructionCount::execute()
{
	
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
				if (insn!=NULL&&insn->GetFallthrough()!=NULL)
				{
					string count_instruction = string("count_instruction");

					addCookbookCallback(insn, count_instruction, true, NULL);
					continue;

					Instruction_t *start = NULL;
					Instruction_t *rsp_save = NULL;
					Instruction_t *rsp_restore = NULL;
					Instruction_t *orig = NULL;
					Instruction_t *call = NULL;

					/*
					 * We are going to insert some
					 * new instructions before the current
					 * one so that we can invoke a callback:
					 *
					 * nop
					 * [save the stack pointer]
					 * [invoke the callback]
					 * [restore the stack pointer]
					 * [original instruction]
					 */
					/*
					 * Now, insert the "start" instruction (nop)
					 * before the original instruction. We
					 * will chain the remainder of our new
					 * instructions from start.
					 *
					 * This function returns a new Instruction_t
					 * for the original instruction which we
					 * want to use from now on.
					 */
					orig = insertAssemblyBefore(getFileIR(), insn, "nop");
					/*
					 * And the function makes insn the nop. So,
					 * since that's our "start" instruction, we
					 * will reassign.
					 */
					start = insn;

					/*
					 * Use the so-called Red Zone
					 * for the invocation of the callback
					 * function.
					 */
					rsp_save = allocateNewInstruction(
						insn->GetAddress()->GetFileID(), insn->GetFunction());
					start->SetFallthrough(rsp_save);
					setAssembly(rsp_save, "lea rsp, [rsp-128]");

					/*
					 * Call the callback.
					 */
					call = allocateNewInstruction(insn->GetAddress()->GetFileID(), 
						insn->GetFunction());
					setAssembly(call, "call 0");
					call->SetComment("nop to call " + count_instruction + ".");	
					addCallbackHandler64(call, count_instruction, 2);

					/*
					 * Chain the rsp save operation
					 * to the call.
					 */
					rsp_save->SetFallthrough(call);

					/*
					 * Restore rsp to its previous location.
					 */
					rsp_restore = addNewAssembly(call, "lea rsp, [rsp+128]");  

					/*
					 * Finish the chain:
					 * call -> rsp restore
					 * rsp restore -> original instruction
					 */
					call->SetFallthrough(rsp_restore);
					rsp_restore->SetFallthrough(orig);
				}
			}
		}
	}
	return 0;
}
