#include "whitelist.hpp"

#include <assert.h>


using namespace libTransform;

Whitelistcall::Whitelistcall(VariantID_t *p_variantID, FileIR_t *p_variantIR, set<std::string> *p_filteredFunctions) : CookbookTransform(p_variantID, p_variantIR, p_filteredFunctions) 
{
	
}

int Whitelistcall::execute()
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
					string whitelist_callback = string("whitelist_open_callback");
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
							!function->GetName().compare(1, strlen("open"), "open")
							)
						{
							/*
							 * Use CookbookTransform's convenience
							 * method to add a callback after the call
							 * to open. The callback will take the open()'s
							 * return value and add it to the whitelist.
							 */
							addCookbookCallback(insn, whitelist_callback, false);
						}
					}
				}
			}
		}
	}
	return 0;
}
