#include "syscall.hpp"

#include <assert.h>


using namespace libTransform;

Syscall::Syscall(VariantID_t *p_variantID, FileIR_t *p_variantIR, set<std::string> *p_filteredFunctions) : CookbookTransform(p_variantID, p_variantIR, p_filteredFunctions) 
{
	
}

int Syscall::execute()
{
#ifndef NO_IDAPRO
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
			if(insn /*&& insn->GetAddress()*/)
			{
				if (insn!=NULL && insn->GetFallthrough()!=NULL) 
				{
					string syscall_callback = string("syscall_callback");
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
						 * method to add a callback before insn.
						 */
						addCookbookCallback(insn, syscall_callback, false);
					}
				}
			}
#ifndef NO_IDAPRO
		}
#endif
	}
	return 0;
}
