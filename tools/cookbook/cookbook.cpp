#include "cookbook.hpp"

#include <assert.h>

using namespace libTransform;

void CookbookTransform::addCookbookCallback(Instruction_t *original, 
	string callback)
{
	Instruction_t *start = NULL;
	Instruction_t *rsp_save = NULL;
	Instruction_t *rsp_restore = NULL;
	Instruction_t *orig = NULL;
	Instruction_t *call = NULL;
	
	start = addNewAssembly("nop");
	rsp_save = allocateNewInstruction(
		original->GetAddress()->GetFileID(), original->GetFunction());
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
	orig = carefullyInsertBefore(original, start);
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
	call = allocateNewInstruction(original->GetAddress()->GetFileID(), 
	original->GetFunction());
	setAssembly(call, "call 0");
	call->SetComment("call " + callback + " before original.");	
	addCallbackHandler64(call, callback, 2);

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
