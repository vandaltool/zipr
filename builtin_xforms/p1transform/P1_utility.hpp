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

#include <irdb-core>
#include <irdb-transform>
#include "globals.h"

using namespace IRDB_SDK;
using namespace std;


Instruction_t* P1_insertAssemblyBefore(FileIR_t* virp, Instruction_t* first, string assembly, Instruction_t *target);
Instruction_t* P1_insertAssemblyBefore(FileIR_t* virp, Instruction_t* first, string assembly);
Instruction_t* P1_insertAssemblyAfter(FileIR_t* virp, Instruction_t* first, string assembly, Instruction_t *target);
Instruction_t* P1_insertAssemblyAfter(FileIR_t* virp, Instruction_t* first, string assembly);
Instruction_t* P1_insertDataBitsAfter(FileIR_t* virp, Instruction_t* first, string dataBits, Instruction_t *target);
Instruction_t* P1_insertDataBitsAfter(FileIR_t* virp, Instruction_t* first, string dataBits);
Instruction_t* P1_insertDataBitsBefore(FileIR_t* virp, Instruction_t* first, string dataBits, Instruction_t *target);
Instruction_t* P1_insertDataBitsBefore(FileIR_t* virp, Instruction_t* first, string dataBits);
Instruction_t* P1_copyInstruction(Instruction_t* instr);
void P1_copyInstruction(Instruction_t* src, Instruction_t* dest);
Instruction_t* P1_allocateNewInstruction(FileIR_t* virp, DatabaseID_t p_fileID,Function_t* func);
Instruction_t* P1_allocateNewInstruction(FileIR_t* virp, Instruction_t *template_instr);
void P1_setInstructionAssembly(FileIR_t* virp,Instruction_t *p_instr, string p_assembly, Instruction_t *p_fallThrough, Instruction_t *p_target);


string getJumpDataBits();
string getJnsDataBits();
string getJzDataBits();
string getJnzDataBits();
string getJecxzDataBits();
string getRetDataBits();
Instruction_t* getHandlerCode(FileIR_t* virp, Instruction_t* fallthrough, mitigation_policy policy, unsigned exitcode );

//The esp offset is allowed to be negative, and is handled properly.
//Returns the pointer for the copied "first" instruction, which is at the
//end of the canary check block of instructions. 
Instruction_t* insertCanaryCheckBefore(FileIR_t* virp,Instruction_t *first, unsigned int canary_val, int ret_offset, Instruction_t *fail_code); 
Instruction_t* insertCanaryZeroAfter(FileIR_t* virp, Instruction_t *first, int esp_offset, Instruction_t *fail_code);

Relocation_t* createNewRelocation(FileIR_t* firp, Instruction_t* insn, string type, int offset);



