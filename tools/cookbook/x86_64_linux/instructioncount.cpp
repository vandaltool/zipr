#include "instructioncount.hpp"

#include "Rewrite_Utility.hpp"

#include <assert.h>

using namespace libTransform;

InstructionCount::InstructionCount(VariantID_t *p_variantID, FileIR_t *p_variantIR, set<std::string> *p_filteredFunctions) : CookbookTransform(p_variantID, p_variantIR, p_filteredFunctions) 
{
	
}

int InstructionCount::execute()
{
#ifndef NO_IDAPRO
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
#else
	set<Instruction_t*> insns = getFileIR()->GetInstructions();
	for (
		set<Instruction_t*>::const_iterator it=insns.begin();
		it!=insns.end();
		++it
		)
	{
#endif
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
					Instruction_t *pushf = NULL;
					Instruction_t *popf = NULL;
					Instruction_t *orig = NULL;
					Instruction_t *call = NULL;

					/*
					 * We are going to insert some
					 * new instructions before the current
					 * one so that we can invoke a callback:
					 *
					 * nop
					 * [save the stack pointer]
					 * [save the flags]
					 * [invoke the callback]
					 * [restore the flags]
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
					 * Save the CPU flags.
					 */
					pushf = allocateNewInstruction(
						insn->GetAddress()->GetFileID(), insn->GetFunction());
					rsp_save->SetFallthrough(pushf);
					setAssembly(pushf, "pushf");

					/*
					 * Call the callback.
					 */
					call = allocateNewInstruction(insn->GetAddress()->GetFileID(), 
						insn->GetFunction());
					setAssembly(call, "call 0");
					call->SetComment("nop to call " + count_instruction + ".");	
					addCallbackHandler64(call, count_instruction, 2);

					/*
					 * Chain the call operation
					 * to the pushf.
					 */
					pushf->SetFallthrough(call);

					/*
					 * Restore the CPU flags.
					 */
					popf = addNewAssembly(call, "popf");
					call->SetFallthrough(popf);

					/*
					 * Restore rsp to its previous location.
					 */
					rsp_restore = addNewAssembly(popf, "lea rsp, [rsp+128]");  

					/*
					 * Finish the chain:
					 * popf -> rsp restore
					 * rsp restore -> original instruction
					 */
					popf->SetFallthrough(rsp_restore);
					rsp_restore->SetFallthrough(orig);
				}
			}
#ifndef NO_IDAPRO
		}
#endif
	}
	return 0;
}
