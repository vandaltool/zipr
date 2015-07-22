#include "instructioncount.hpp"

#include <assert.h>

using namespace libTransform;

InstructionCount::InstructionCount(VariantID_t *p_variantID, FileIR_t *p_variantIR, set<std::string> *p_filteredFunctions) : Transform(p_variantID, p_variantIR, p_filteredFunctions) 
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

					Instruction_t *start = NULL;
					Instruction_t *rsp_save = NULL;
					Instruction_t *rsp_restore = NULL;
					Instruction_t *orig = NULL;
					Instruction_t *call = NULL;
					
					start = addNewAssembly("nop");
					rsp_save = allocateNewInstruction(
						insn->GetAddress()->GetFileID(), insn->GetFunction());
					start->SetFallthrough(rsp_save);

					/*
					 * Now, insert the "start" instruction
					 * before the original instruction. We
					 * will chain the remainder of our new
					 * instructions from start.
					 *
					 * This function returns a new Instruction_t
					 * for the original instruction which we
					 * want to use from now on.
					 */
					orig = carefullyInsertBefore(insn, start);
					start->SetFallthrough(rsp_save);

					/*
					 * Use the so-called Red Zone
					 * for the invocation of the callback
					 * function.
					 */
					setAssembly(rsp_save, "lea rsp, [rsp-128]");

					/*
					 * Call the callback.
					 */
					call = allocateNewInstruction(insn->GetAddress()->GetFileID(), 
						insn->GetFunction());
					setAssembly(call, "call 0");
					call->SetComment("call " + count_instruction + " before original.");	
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
