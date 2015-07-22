#include "functioncall.hpp"

#include <assert.h>


using namespace libTransform;

Functioncall::Functioncall(VariantID_t *p_variantID, FileIR_t *p_variantIR, set<std::string> *p_filteredFunctions) : CookbookTransform(p_variantID, p_variantIR, p_filteredFunctions) 
{
	
}

int Functioncall::execute()
{
	for (
		set<Function_t*>::const_iterator itf=getFileIR()->GetFunctions().begin();
		itf!=getFileIR()->GetFunctions().end();
		++itf
		)
	{
		Function_t* func=*itf;
		for (
			set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
			it!=func->GetInstructions().end();
			++it
			)
		{
			Instruction_t* insn = *it;
			if(insn /*&& insn->GetAddress()*/)
			{
				if (insn!=NULL && insn->GetFallthrough()!=NULL) 
				{
					string functioncall_callback = string("functioncall_callback");
					unsigned char dbs[1] = {0};

					/*
					 * An Instruction_t has a convenience
					 * method for accessing the instruction's
					 * data. For an instruction, the op code
					 * is always in either the first or first
					 * and second byte.
					 */
					dbs[0] = insn->GetDataBits()[0];

					/*
					 * call is 0xe8 on x64. Check for
					 * that type of instruction and
					 * double check that it has a target.
					 */
					if (dbs[0] == 0xe8 &&
						insn->GetTarget()
						)
					{
						/*
						 * Use the call instruction's target 
						 * to get information about the target
						 * function using the libtransform API.
						 */
						Instruction_t *target = insn->GetTarget();
						Function_t *function = target->GetFunction();
						
						if (function != NULL &&
							/*
							 * Use Function_t's GetName() function
							 * to determine if the target function
							 * is the one we want to hook.
							 *
							 * NB: The disassembler puts a . before
							 * the actual function name. So, we
							 * use the extended version of compare()
							 * to compensate.
							 */
							!function->GetName().compare(1, strlen("write"), "write")
							)
						{
							/*
							 * Use CookbookTransform's convenience
							 * method to add a callback before insn.
							 */
							addCookbookCallback(insn, functioncall_callback);
						}
					}
				}
			}
		}
	}
	return 0;
}
