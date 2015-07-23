#include "cookbook.hpp"

#include "Rewrite_Utility.hpp"
#include <assert.h>

using namespace libTransform;
void CookbookTransform::addCookbookCallback(Instruction_t *original, 
	string callback,
	bool before,
	void *extra)
{
	Instruction_t *start = NULL;
	Instruction_t *rsp_save = NULL;
	Instruction_t *rsp_restore = NULL;
	Instruction_t *orig = NULL;
	Instruction_t *call = NULL;
	Instruction_t *push = NULL;
	char pushAsm[1024] = {0,};

	/*
	 * We are going to insert some
	 * new instructions before/after the current
	 * one so that we can invoke a callback:
	 *
	 * nop
	 * [save the stack pointer]
	 * [invoke the callback]
	 * [restore the stack pointer]
	 * [original instruction]
	 */

	if (before)
	{
		/*
		 * Insert the "start" instruction (nop)
		 * before the original instruction. We
		 * will chain the remainder of our new
		 * instructions from start.
		 *
		 * This function returns a new Instruction_t
		 * for the original instruction which we
		 * want to use from now on.
		 */
		orig = insertAssemblyBefore(getFileIR(), original, "nop");
		/*
		 * And the function makes original the nop. So,
		 * since that's our "start" instruction, we
		 * will reassign.
		 */
		start = original;
	}
	else
	{
		start = addNewAssembly("nop");
	}

	/*
	 * Use the so-called Red Zone
	 * for the invocation of the callback
	 * function.
	 */
	rsp_save = allocateNewInstruction(
		original->GetAddress()->GetFileID(), original->GetFunction());
	start->SetFallthrough(rsp_save);
	setAssembly(rsp_save, "lea rsp, [rsp-128]");

	/*
	 * Create a push 0x<value> instruction.
	 */
	sprintf(pushAsm, "push 0x%08x\n", extra);
	/*
	 * NB: addNewAssembly() sets rsp_save's
	 * fallthrough address to the new instruction
	 * by default.
	 */
	push = addNewAssembly(rsp_save, pushAsm);

	/*
	 * Call the callback.
	 */
	call = allocateNewInstruction(original->GetAddress()->GetFileID(), 
		original->GetFunction());
	setAssembly(call, "call 0");
	call->SetComment("call " + callback + " before original.");	
	addCallbackHandler64(call, callback, 2);

	/*
	 * Chain the push operation
	 * to the call.
	 */
	push->SetFallthrough(call);

	/*
	 * Restore rsp to its previous location.
	 * Jump over the 8 bytes we
	 * used for the pointer to
	 * extra.
	 */
	rsp_restore = addNewAssembly(call, "lea rsp, [rsp+128+8]");

	/*
	 * Finish the chain:
	 * call -> rsp restore
	 */
	call->SetFallthrough(rsp_restore);

	/*
	 * The last step depends on whether
	 * we are doing the callback before
	 * or after the instruction.
	 */
	if (before)
	{
		/*
		 * rsp restore -> original instruction
		 */
		rsp_restore->SetFallthrough(orig);
	} 
	else 
	{
		/*
		 * Link the end of our chain 
		 * to the original instruction's 
		 * fall through. Then, link the
		 * original instruction's fall
		 * through to our chain:
		 *
		 * [original]
		 * [callback invocation code]
		 * [original's fall through]
		 */
		rsp_restore->SetFallthrough(original->GetFallthrough());
		original->SetFallthrough(start);
	}
}

int CookbookTransform::execute()
{
	/*
	 * Our execution function is a series of
	 * nested loops that will ultimately mean
	 * that we will see each instruction in the
	 * program:
	 *
	 * for each function
	 *  for each instruction
	 */
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
			 * Log the opcode and the instruction's
			 * comments. These statements will
			 * show up in the peasoupified program's
			 * runtime logs/ directory.
			 */
			printf("%x %x:", dbs[0], dbs[1]);
			cout << insn->GetComment() << endl;
#ifndef NO_IDAPRO
		}
#endif
	}
	return 0;
}
