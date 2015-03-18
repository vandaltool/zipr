#include "cgc_hlx.hpp"
#include "Rewrite_Utility.hpp"
#include <stdlib.h>

#define CINDERELLA_MALLOC	"cinderella::malloc"
#define CINDERELLA_ALLOCATE	"cinderella::allocate"

using namespace std;
using namespace libIRDB;

Function_t* HLX_Instrument::findFunction(string p_functionName)
{
	for(set<Function_t*>::iterator it=m_firp->GetFunctions().begin(); 
		it!=m_firp->GetFunctions().end(); 
		++it)
	{
		Function_t* func=*it;

		if(func && func->GetName() == p_functionName)
			return func;
	}

	return NULL;
}

// pad argument #1 of function, assume it's the size
bool HLX_Instrument::padSize(Function_t* const p_func)
{
	assert(p_func);

	Instruction_t *entry = p_func->GetEntryPoint();

	if (!entry)
		return false; 

	cout << "padding function: " << p_func->GetName() << " at entry point: 0x" << hex << entry->GetAddress()->GetVirtualOffset() << dec << endl;

	/*
	* Pad by 1/16 (nb: CGC scoring function give a 10% allowance)      
	*
	*    eax <-- [esp + 4]      ; get the size (1st argument)
	*    eax >>= 4              ; divide by 16 to get the padding
	*    eax + 1                ; add 1 to padding in case the size < 16
    *    add [esp+4], eax       ; add padding to size
	*/

	Instruction_t* instr = NULL;
	Instruction_t* orig = NULL;

	orig  = insertAssemblyBefore(m_firp, entry, "mov eax, [esp+4]");
	entry->SetComment("pad malloc sequence");

	instr = insertAssemblyAfter (m_firp, entry, "shr eax, 4");
	instr = insertAssemblyAfter (m_firp, instr, "add eax, 256");
	instr = insertAssemblyAfter (m_firp, instr, "add [esp+4], eax");
	instr->SetFallthrough(orig);

	return true;
}

bool HLX_Instrument::execute()
{
	bool success=false;

	Function_t *cinderella_malloc = findFunction(CINDERELLA_MALLOC);
	Function_t *cinderella_allocate = findFunction(CINDERELLA_ALLOCATE);

	if (cinderella_malloc)
		success = padSize(cinderella_malloc);

	if (cinderella_allocate)
		success = padSize(cinderella_allocate);

	return success;
}
