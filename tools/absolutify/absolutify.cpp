#include "absolutify.hpp"

#include "Rewrite_Utility.hpp"
#include <assert.h>
#include <stdexcept>

using namespace libTransform;
using namespace ELFIO;
using namespace libIRDB;

Absolutify::Absolutify(FileIR_t *p_variantIR) :
	Transform(NULL, p_variantIR, NULL)
{
	
}

Absolutify::~Absolutify() 
{
}

bool arg_has_relative(const ARGTYPE &arg)
{
	/* if it's relative memory, watch out! */
	if(arg.ArgType&MEMORY_TYPE)
		if(arg.ArgType&RELATIVE_)
			return true;
	
	return false;
}

bool arg_has_constant(const ARGTYPE &arg)
{
	/* if it's relative memory, watch out! */
	if(arg.ArgType&CONSTANT_TYPE)
		return true;
	
	return false;
}
int Absolutify::execute()
{
/*
	for(
	  set<Function_t*>::const_iterator itf=getFileIR()->GetFunctions().begin();
	  itf!=getFileIR()->GetFunctions().end();
	  ++itf
	  )
	{
		Function_t* func=*itf;
*/
		for(
		  //set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
		  set<Instruction_t*>::const_iterator it=getFileIR()->GetInstructions().begin();
		  //it!=func->GetInstructions().end();
		  it!=getFileIR()->GetInstructions().end();
		  ++it)
		{
			DISASM disasm;
			Instruction_t *insn = *it;
			ARGTYPE* the_arg=NULL;
			string updated_asm;
			size_t open_bracket_index = 0;

			insn->Disassemble(disasm);

			int is_rel= arg_has_relative(disasm.Argument1) ||
			            arg_has_relative(disasm.Argument2) || 
									arg_has_relative(disasm.Argument3);
			if (!is_rel)
				continue;

			if(arg_has_relative(disasm.Argument1))
				the_arg=&disasm.Argument1;
			if(arg_has_relative(disasm.Argument2))
				the_arg=&disasm.Argument2;
			if(arg_has_relative(disasm.Argument3))
				the_arg=&disasm.Argument3;
			assert(the_arg);

			updated_asm = string(disasm.CompleteInstr);

			cout << "Original Disassembled: " << updated_asm << endl;

			while (string::npos != 
			      (open_bracket_index = updated_asm.find("[", open_bracket_index))
						)
			{
				updated_asm.replace(open_bracket_index, 1, "[abs ");
				open_bracket_index++;
			}
			cout << "updated Assembly: " << updated_asm << endl;

			insn->Assemble(updated_asm);
			
			insn->Disassemble(disasm);
			updated_asm = string(disasm.CompleteInstr);

			cout << "Updated Disassembled: " << updated_asm << endl;
		}
		/*
	}
	*/
	return true;
}
