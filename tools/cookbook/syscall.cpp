#include "syscall.hpp"

#include <assert.h>


using namespace libTransform;

SyscallPlay::SyscallPlay(VariantID_t *p_variantID, FileIR_t *p_variantIR, set<std::string> *p_filteredFunctions) : CookbookTransform(p_variantID, p_variantIR, p_filteredFunctions) 
{
	
}

int SyscallPlay::execute()
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
			if(insn /*&& insn->GetAddress()*/)
			{
				if (insn!=NULL&&insn->GetFallthrough()!=NULL) 
				{
					string syscall_callback = string("syscall_callback");

					unsigned char dbs[2] = {0,};
					dbs[0] = insn->GetDataBits()[0];
					dbs[1] = insn->GetDataBits()[1];
					/*
					 * syscall is two byte instruction
					 * on x64.
					 */
					if (dbs[0] == 0x0f && dbs[1] == 0x05 ) 
					{
						addCookbookCallback(insn, syscall_callback);
					}
				}
			}
		}
	}
	return 0;
}
