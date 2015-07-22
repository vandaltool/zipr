#include "checkwhitelist.hpp"

#include <assert.h>


using namespace libTransform;

Checkwhitelist::Checkwhitelist(VariantID_t *p_variantID, FileIR_t *p_variantIR, set<std::string> *p_filteredFunctions) : CookbookTransform(p_variantID, p_variantIR, p_filteredFunctions) 
{
	
}

int Checkwhitelist::execute()
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
					string syscall_callback = string("whitelist_syscall_check");
					unsigned char dbs[2] = {0,};

					/*
					 * An Instruction_t has a convenience
					 * method for accessing the instruction's
					 * data. For an instruction, the op code
					 * is always in either the first or first
					 * and second byte.
					 */
					dbs[0] = insn->GetDataBits()[0];
					dbs[1] = insn->GetDataBits()[1];

					/*
					 * syscall is two byte instruction
					 * on x64.
					 */
					if (dbs[0] == 0x0f && dbs[1] == 0x05 ) 
					{
						/*
						 * Use CookbookTransform's convenience
						 * method to add a callback before the systemcall.
						 * This callback will determine if a write()
						 * is being called on a non-whitelisted
						 * file descriptor.
						 */
						addCookbookCallback(insn, syscall_callback);
					}
				}
			}
		}
	}
	return 0;
}
