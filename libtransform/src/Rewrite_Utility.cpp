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

#include "Rewrite_Utility.hpp"

#include <libIRDB-core.hpp>

// Copied from PnTransform
// @todo: create a utility library with the one interface

using namespace std;

namespace IRDBUtility
{

void setExitCode(FileIR_t* virp, IRDB_SDK::Instruction_t* exit_code);



void setInstructionsDetails(FileIR_t* virp, IRDB_SDK::Instruction_t *p_instr, string p_dataBits, IRDB_SDK::Instruction_t *p_fallThrough, IRDB_SDK::Instruction_t *p_target)
{
        if (p_instr == NULL) return;

        p_instr->setDataBits(p_dataBits);
        p_instr->setComment(p_instr->getDisassembly());
        p_instr->setFallthrough(p_fallThrough);
        p_instr->setTarget(p_target);

	auto real_virp=dynamic_cast<libIRDB::FileIR_t*>(virp);
	assert(real_virp);

        real_virp->GetAddresses().insert(p_instr->getAddress());
        real_virp->GetInstructions().insert(p_instr);
}


//For all insertBefore functions:
//The "first" instruction will have its contents replaced and a duplicate of "first" will be in the follow of first. 
//This duplicate is returned since the user already has a pointer to first.
//To insert before an instruction is the same as modifying the original instruction, and inserting after it
//a copy of the original instruction 
IRDB_SDK::Instruction_t* insertAssemblyBefore(IRDB_SDK::FileIR_t* virp, IRDB_SDK::Instruction_t* first, string assembly, IRDB_SDK::Instruction_t *target)
{
	IRDB_SDK::Instruction_t* next = copyInstruction(virp,first);

	//In case the fallthrough is null, generate spri has to have a 
	//place to jump, which is determined by the original address.
	//This code is not placed in copyInstruction since this is only needed
	//when inserting before
	next->setOriginalAddressID(first->getOriginalAddressID());
	//"Null" out the original address (it should be as if the instruction was not in the database).
	first->setOriginalAddressID(BaseObj_t::NOT_IN_DATABASE);
	auto real_first=dynamic_cast<libIRDB::Instruction_t*>(first);
	assert(real_first);
	real_first->GetRelocations().clear();
        first->setIBTargets(NULL);
	//Note that the instruction just inserted should have the same exception handling
        //info as the instructions immediately around it.
        //Thus the exception handling information (EhCallSite and EhProgram) are kept the 
        //same from the copy of first (unlike with relocations and IBT's).

	virp->changeRegistryKey(first,next);
	setInstructionAssembly(virp,first,assembly,next,target);

	return next;
}

IRDB_SDK::Instruction_t* insertAssemblyBefore(FileIR_t* virp, IRDB_SDK::Instruction_t* first, string assembly)
{
	return insertAssemblyBefore(virp,first,assembly,NULL);
}


IRDB_SDK::Instruction_t* insertDataBitsBefore(FileIR_t* virp, IRDB_SDK::Instruction_t* first, string dataBits)
{
        return insertDataBitsBefore(virp,first,dataBits,NULL);
}

IRDB_SDK::Instruction_t* insertDataBitsBefore(FileIR_t* virp, IRDB_SDK::Instruction_t* first, string dataBits, IRDB_SDK::Instruction_t *target)
{
	IRDB_SDK::Instruction_t* next = copyInstruction(virp,first);

        //In case the fallthrough is null, generate spri has to have a 
        //place to jump, which is determined by the original address.
        //This code is not placed in copyInstruction since this is only needed
        //when inserting before
        next->setOriginalAddressID(first->getOriginalAddressID());
        //"Null" out the original address (it should be as if the instruction was not in the database).
        first->setOriginalAddressID(BaseObj_t::NOT_IN_DATABASE);
	auto real_first=dynamic_cast<libIRDB::Instruction_t*>(first);
	assert(real_first);
        real_first->GetRelocations().clear();
	first->setIBTargets(NULL);
	//Note that the instruction just inserted should have the same exception handling
	//info as the instructions immediately around it.
	//Thus the exception handling information (EhCallSite and EhProgram) are kept the 
	//same from the copy of first (unlike with relocations and IBT's).

	virp->changeRegistryKey(first,next);
        setInstructionsDetails(virp,first,dataBits,next,target);

        return next;
}

IRDB_SDK::Instruction_t* insertAssemblyAfter(IRDB_SDK::FileIR_t* virp, IRDB_SDK::Instruction_t* first, string assembly, IRDB_SDK::Instruction_t *target)
{
	IRDB_SDK::Instruction_t *new_instr = allocateNewInstruction(virp,first);
        setInstructionAssembly(virp,new_instr,assembly,first->getFallthrough(), target);
        first->setFallthrough(new_instr);
        return new_instr;
}

IRDB_SDK::Instruction_t* insertAssemblyAfter(IRDB_SDK::FileIR_t* virp, IRDB_SDK::Instruction_t* first, string assembly)
{
        return insertAssemblyAfter(virp,first,assembly,NULL);

}

IRDB_SDK::Instruction_t* insertDataBitsAfter(IRDB_SDK::FileIR_t* virp, IRDB_SDK::Instruction_t* first, string dataBits, IRDB_SDK::Instruction_t *target)
{
	IRDB_SDK::Instruction_t *new_instr = allocateNewInstruction(virp,first);
        setInstructionsDetails(virp,new_instr,dataBits,first->getFallthrough(), target);
        first->setFallthrough(new_instr);

        return new_instr;
}

IRDB_SDK::Instruction_t* insertDataBitsAfter(IRDB_SDK::FileIR_t* virp, IRDB_SDK::Instruction_t* first, string dataBits)
{
        return insertDataBitsAfter(virp,first,dataBits,NULL);
}

IRDB_SDK::Instruction_t* addNewDatabits(IRDB_SDK::FileIR_t* firp, IRDB_SDK::Instruction_t *p_instr, string p_bits)
{
	IRDB_SDK::Instruction_t* newinstr;
        if (p_instr)
                newinstr = allocateNewInstruction(firp,p_instr->getAddress()->getFileID(), p_instr->getFunction());
        else
                newinstr = allocateNewInstruction(firp,firp->getFile()->getFileID(), NULL);

        newinstr->setDataBits(p_bits);

        if (p_instr)
        {
                newinstr->setFallthrough(p_instr->getFallthrough());
                p_instr->setFallthrough(newinstr);
        }

        return newinstr;
}

IRDB_SDK::Instruction_t* addNewAssembly(FileIR_t* firp, IRDB_SDK::Instruction_t *p_instr, string p_asm)
{
	IRDB_SDK::Instruction_t* newinstr;
        if (p_instr)
                newinstr = allocateNewInstruction(firp,p_instr->getAddress()->getFileID(), p_instr->getFunction());
        else
                newinstr = allocateNewInstruction(firp,firp->getFile()->getFileID(), NULL);

        firp->registerAssembly(newinstr, p_asm);

        if (p_instr)
        {
                newinstr->setFallthrough(p_instr->getFallthrough());
                p_instr->setFallthrough(newinstr);
        }

        return newinstr;
}






//Does not insert into any variant
IRDB_SDK::Instruction_t* copyInstruction(IRDB_SDK::Instruction_t* instr)
{
	IRDB_SDK::Instruction_t* cpy = new libIRDB::Instruction_t();

	copyInstruction(instr,cpy);

	return cpy;
}

IRDB_SDK::Instruction_t* copyInstruction(IRDB_SDK::FileIR_t* virp, IRDB_SDK::Instruction_t* instr)
{
	IRDB_SDK::Instruction_t* cpy = allocateNewInstruction(virp,instr);

	copyInstruction(instr,cpy);

	return cpy;
}

void copyInstruction(IRDB_SDK::Instruction_t* src, IRDB_SDK::Instruction_t* dest)
{
	auto real_dest=dynamic_cast<libIRDB::Instruction_t*>(dest);
	dest->setDataBits(src->getDataBits());
	dest->setComment(src->getComment());
	dest->setCallback(src->getCallback());
	dest->setFallthrough(src->getFallthrough());
	dest->setTarget(src->getTarget());
        dest->setIBTargets(src->getIBTargets()); 
	real_dest->GetRelocations()=src->getRelocations();
        dest->setEhProgram(src->getEhProgram());
	dest->setEhCallSite(src->getEhCallSite());
}

IRDB_SDK::Instruction_t* allocateNewInstruction(IRDB_SDK::FileIR_t* virp, IRDB_SDK::DatabaseID_t p_fileID,IRDB_SDK::Function_t* func)
{
	auto instr = new libIRDB::Instruction_t();
	auto a     = new libIRDB::AddressID_t();

	a->setFileID(p_fileID);

	instr->setFunction(func);
	instr->setAddress(a);
	if(func)
	{
		auto real_func=dynamic_cast<libIRDB::Function_t*>(func);
		assert(func);
		real_func->GetInstructions().insert(instr);
	}

	auto real_virp=dynamic_cast<libIRDB::FileIR_t*>(virp);
	assert(real_virp);
	real_virp->GetInstructions().insert(instr);
	real_virp->GetAddresses().insert(a);

//	inserted_instr[func].insert(instr);
//	inserted_addr[func].insert(a);
	
	return instr;
}

IRDB_SDK::Instruction_t* allocateNewInstruction(IRDB_SDK::FileIR_t* virp, IRDB_SDK::Instruction_t *template_instr)
{
	Function_t *func = template_instr->getFunction();
	DatabaseID_t fileID = template_instr->getAddress()->getFileID();
	return allocateNewInstruction(virp, fileID, func);
}

void setInstructionAssembly(IRDB_SDK::FileIR_t* virp,IRDB_SDK::Instruction_t *p_instr, string p_assembly, IRDB_SDK::Instruction_t *p_fallThrough, IRDB_SDK::Instruction_t *p_target)
{
	if (p_instr == NULL) return;
	
	///TODO: what if bad assembly?
	virp->registerAssembly(p_instr,p_assembly);
	p_instr->setComment(p_assembly); 
	p_instr->setFallthrough(p_fallThrough); 
	p_instr->setTarget(p_target); 

	auto real_virp=dynamic_cast<libIRDB::FileIR_t*>(virp);
	assert(real_virp);
	real_virp->GetAddresses().insert(p_instr->getAddress());
	real_virp->GetInstructions().insert(p_instr);
}


}
