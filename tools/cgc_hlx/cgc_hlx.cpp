#include "cgc_hlx.hpp"
#include "Rewrite_Utility.hpp"
#include <stdlib.h>

/*
*  HLX: Heap Layout Transform
* 
*  Pad malloc and/or allocate
*
*/

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
bool HLX_Instrument::padSize(Function_t* const p_func, const int padding, const int shr_factor)
{
	assert(p_func);

	Instruction_t *entry = p_func->GetEntryPoint();

	if (!entry)
	{
		cerr << "function: " << p_func->GetName() << " has no entry point defined" << endl;
		return false; 
	}

	cout << "padding function: " << p_func->GetName() << " at: 0x" << hex << entry->GetAddress()->GetVirtualOffset() << dec << "padding: " << padding << " shr_factor: " << shr_factor << endl;

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

	orig = insertAssemblyBefore(m_firp, entry, "mov eax, [esp+4]"); 
	entry->SetComment("pad malloc/allocate sequence");

	char buf[1024];

	if (shr_factor > 0)
	{
		sprintf(buf, "shr eax, %d", shr_factor); 
		instr = insertAssemblyAfter(m_firp, entry, buf);
	}

	sprintf(buf, "add eax, %d", padding); // in bytes
	
	if (shr_factor > 0)
		instr = insertAssemblyAfter(m_firp, instr, buf);
	else
		instr = insertAssemblyAfter(m_firp, entry, buf);

	instr = insertAssemblyAfter(m_firp, instr, "add [esp+4], eax");
	instr->SetFallthrough(orig);

	return true;
}

bool HLX_Instrument::execute()
{
	bool one_success=false;

	if (mallocPaddingEnabled())
	{
		Function_t *cinderella_malloc = findFunction(CINDERELLA_MALLOC);
		if (cinderella_malloc)
		{
			cout << "found " << CINDERELLA_MALLOC << endl;
			if (padSize(cinderella_malloc, getMallocPadding(), getShiftRightFactor()))
			{
				one_success = true;
				cout << CINDERELLA_MALLOC << " padded successfully: " << getMallocPadding() << " bytes" << endl;
			}
		}
		else
		{
			cout << CINDERELLA_MALLOC << " not found" << endl;
		}
	}

	if (allocatePaddingEnabled())
	{
		Function_t *cinderella_allocate = findFunction(CINDERELLA_ALLOCATE);

		if (cinderella_allocate)
		{	
			cout << "found " << CINDERELLA_ALLOCATE << endl;
			if (padSize(cinderella_allocate, getAllocatePadding()))
			{
				one_success = true;
				cout << CINDERELLA_ALLOCATE << " padded successfully: " << getAllocatePadding() << " bytes" << endl;
			}
		}
		else
		{
			cout << CINDERELLA_ALLOCATE << " not found" << endl;
		}
	}

	return one_success;
}
