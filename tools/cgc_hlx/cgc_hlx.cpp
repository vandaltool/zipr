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
#define CINDERELLA_DEALLOCATE	"cinderella::deallocate"

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
bool HLX_Instrument::padSizeOnAllocation(Function_t* const p_func, const int padding, const int shr_factor)
{
	assert(p_func);

	Instruction_t *entry = p_func->GetEntryPoint();

	if (!entry)
	{
		cerr << "function: " << p_func->GetName() << " has no entry point defined" << endl;
		return false; 
	}

	cout << "padding function: " << p_func->GetName() << " at: 0x" << hex << entry->GetAddress()->GetVirtualOffset() << dec << " padding: " << padding << " shr_factor: " << shr_factor << endl;

	/*
	*  mov eax, [esp+4]
	*  shr eax, k
	*  add eax, p
	*  mov [esp+4], eax
	*
	*  e.g.: size: 256 malloc padding: 32, shift factor = 5
	*     mov eax, [esp+4]    ; 256
	*     shr eax, 5          ; size << 5               = 8
	*     add eax, 32         ; size << 5 + 32          = 40
	*     add [esp+4], eax    ; size = size + size << 5 + 32   = 256 + 40 = 296
	*  
	*  e.g.: size: 4096   padding: 4096  shift factor = 32 
	*     mov eax, [esp+4]    ; 4096
	*     shr eax, 32         ; size << 32               = 0
	*     add eax, 4096       ; size << 32 + 4096        = 4096
	*     add [esp+4], eax    ; size = size + size << 32 + 4096 = 8192
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

// pad argument #2 of function, assume it's the size
bool HLX_Instrument::padSizeOnDeallocation(Function_t* const p_func, const int padding)
{
	assert(p_func);

	Instruction_t *entry = p_func->GetEntryPoint();

	if (!entry)
	{
		cerr << "function: " << p_func->GetName() << " has no entry point defined" << endl;
		return false; 
	}

	cout << "padding function: " << p_func->GetName() << " at: 0x" << hex << entry->GetAddress()->GetVirtualOffset() << dec << "padding: " << padding << endl;

	char buf[1024];

	/*
	*    eax <-- [esp + 8]      ; get the size (1st argument)
	*    add eax, padding       ; compute new size
        *    add [esp+8], eax       ; set to new size
	*/

	Instruction_t* instr = NULL;
	Instruction_t* orig = NULL;

	orig = insertAssemblyBefore(m_firp, entry, "mov eax, [esp+8]"); 
	entry->SetComment("pad deallocate");

	sprintf(buf, "add eax, %d", padding); // in bytes
	instr = insertAssemblyAfter(m_firp, entry, buf);

	instr = insertAssemblyAfter(m_firp, instr, "mov [esp+8], eax");
	instr->SetFallthrough(orig);

	return true;
}

bool HLX_Instrument::execute()
{
	bool success=false;

	if (mallocPaddingEnabled())
	{
		Function_t *cinderella_malloc = findFunction(CINDERELLA_MALLOC);
		if (cinderella_malloc)
		{
			cout << "found " << CINDERELLA_MALLOC << endl;
			if (padSizeOnAllocation(cinderella_malloc, getMallocPadding(), getShiftRightFactor()))
			{
				success = true;
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
		Function_t *cinderella_deallocate = findFunction(CINDERELLA_DEALLOCATE);

		if (cinderella_allocate)
		{	
			cout << "found " << CINDERELLA_ALLOCATE << endl;

			if (padSizeOnAllocation(cinderella_allocate, getAllocatePadding()))
			{
				success = true;
				cout << CINDERELLA_ALLOCATE << " padded successfully: " << getAllocatePadding() << " bytes" << endl;
#ifdef DO_NOT_USE
				if (cinderella_deallocate && padSizeOnDeallocation(cinderella_deallocate, getAllocatePadding()))
				{
					cout << "found " << CINDERELLA_DEALLOCATE << endl;
					cout << CINDERELLA_DEALLOCATE << " padded successfully: " << getAllocatePadding() << " bytes" << endl;
				}
#endif
			}
		}
		else
		{
			cout << CINDERELLA_ALLOCATE << " not found" << endl;
		}
	}

	return success;
}
