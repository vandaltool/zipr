/*
 * Copyright (c) 2013, 2014 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

#include <libIRDB-core.hpp>

using namespace libIRDB;
using namespace std;

namespace IRDBUtility {
// make sure these match values in detector_handlers.h in the strata library
enum mitigation_policy 
{
	P_NONE=0, 
	P_CONTINUE_EXECUTION, 
	P_CONTROLLED_EXIT, 
	P_CONTINUE_EXECUTION_SATURATING_ARITHMETIC, 
	P_CONTINUE_EXECUTION_WARNONLY
};


//The "first" instruction will have its contents replaced and a duplicate of "first" will be in the follow of first. 
//This duplicate is returned since the user already has a pointer to first. 
Instruction_t* insertAssemblyBefore(FileIR_t* virp, Instruction_t* first, string assembly, Instruction_t *target);
Instruction_t* insertAssemblyBefore(FileIR_t* virp, Instruction_t* first, string assembly);
Instruction_t* insertDataBitsBefore(FileIR_t* virp, Instruction_t* first, string dataBits, Instruction_t *target);
Instruction_t* insertDataBitsBefore(FileIR_t* virp, Instruction_t* first, string dataBits);

// insert new instructions after, diff types.
Instruction_t* insertAssemblyAfter(FileIR_t* virp, Instruction_t* first, string assembly, Instruction_t *target);
Instruction_t* insertAssemblyAfter(FileIR_t* virp, Instruction_t* first, string assembly);
Instruction_t* insertDataBitsAfter(FileIR_t* virp, Instruction_t* first, string dataBits, Instruction_t *target);
Instruction_t* insertDataBitsAfter(FileIR_t* virp, Instruction_t* first, string dataBits);

// add new instructions of diff types.
Instruction_t* addNewDatabits(FileIR_t* firp, Instruction_t *p_instr, string p_bits);
Instruction_t* addNewAssembly(FileIR_t* firp, Instruction_t *p_instr, string p_asm);


//Does not insert into any variant, just copies data about the instruction itself, see the copyInstruction(src,dest) to see what is copied. 
Instruction_t* copyInstruction(Instruction_t* instr);

Instruction_t* copyInstruction(FileIR_t* virp, Instruction_t* instr);
//copy src to destination
void copyInstruction(Instruction_t* src, Instruction_t* dest);

Instruction_t* allocateNewInstruction(FileIR_t* virp, db_id_t p_fileID,Function_t* func);
Instruction_t* allocateNewInstruction(FileIR_t* virp, Instruction_t *template_instr);
void setInstructionAssembly(FileIR_t* virp,Instruction_t *p_instr, string p_assembly, Instruction_t *p_fallThrough, Instruction_t *p_target);
Instruction_t* getHandlerCode(FileIR_t* virp, Instruction_t* fallthrough, mitigation_policy policy );

}
