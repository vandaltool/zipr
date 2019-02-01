
#include <irdb-transform>
#include <libIRDB-core.hpp>

// Copied from PnTransform
// @todo: create a utility library with the one interface


using namespace std;
using namespace IRDB_SDK;

void copyInstruction(Instruction_t* src, Instruction_t* dest);
Instruction_t* copyInstruction(FileIR_t* virp, Instruction_t* instr);
Instruction_t* allocateNewInstruction(FileIR_t* virp, DatabaseID_t p_fileID,Function_t* func);
Instruction_t* allocateNewInstruction(FileIR_t* virp, Instruction_t *template_instr);



void setInstructionsDetails(FileIR_t* virp, Instruction_t *p_instr, string p_dataBits, Instruction_t *p_fallThrough, Instruction_t *p_target)
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
Instruction_t* IRDB_SDK::insertAssemblyBefore(FileIR_t* virp, Instruction_t* first, string assembly, Instruction_t *target)
{
	Instruction_t* next = copyInstruction(virp,first);

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
	IRDB_SDK::setInstructionAssembly(virp,first,assembly,next,target);

	return next;
}

#if 0
Instruction_t* insertAssemblyBefore(FileIR_t* virp, Instruction_t* first, string assembly)
{
	return insertAssemblyBefore(virp,first,assembly,NULL);
}


Instruction_t* insertDataBitsBefore(FileIR_t* virp, Instruction_t* first, string dataBits)
{
        return insertDataBitsBefore(virp,first,dataBits,NULL);
}
#endif

Instruction_t* IRDB_SDK::insertDataBitsBefore(FileIR_t* virp, Instruction_t* first, string dataBits, Instruction_t *target)
{
	Instruction_t* next = copyInstruction(virp,first);

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

Instruction_t* IRDB_SDK::insertAssemblyAfter(FileIR_t* virp, Instruction_t* first, string assembly, Instruction_t *target)
{
	Instruction_t *new_instr = allocateNewInstruction(virp,first);
	IRDB_SDK::setInstructionAssembly(virp,new_instr,assembly,first->getFallthrough(), target);
        first->setFallthrough(new_instr);
        return new_instr;
}

#if 0
Instruction_t* insertAssemblyAfter(FileIR_t* virp, Instruction_t* first, string assembly)
{
        return insertAssemblyAfter(virp,first,assembly,NULL);

}
#endif

Instruction_t* IRDB_SDK::insertDataBitsAfter(FileIR_t* virp, Instruction_t* first, string dataBits, Instruction_t *target)
{
	Instruction_t *new_instr = allocateNewInstruction(virp,first);
        setInstructionsDetails(virp,new_instr,dataBits,first->getFallthrough(), target);
        first->setFallthrough(new_instr);

        return new_instr;
}

#if 0
Instruction_t* insertDataBitsAfter(FileIR_t* virp, Instruction_t* first, string dataBits)
{
        return insertDataBitsAfter(virp,first,dataBits,NULL);
}
#endif

Instruction_t* IRDB_SDK::addNewDataBits(FileIR_t* firp, Instruction_t *p_instr, string p_bits)
{
	Instruction_t* newinstr;
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

Instruction_t* IRDB_SDK::addNewDataBits(FileIR_t* firp, string p_bits)
{
	return IRDB_SDK::addNewDataBits(firp,nullptr,p_bits);
}


Instruction_t* IRDB_SDK::addNewAssembly(FileIR_t* firp, Instruction_t *p_instr, string p_asm)
{
	Instruction_t* newinstr;
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

Instruction_t* IRDB_SDK::addNewAssembly(FileIR_t* firp, string p_asm)
{
	return IRDB_SDK::addNewAssembly(firp,nullptr,p_asm);
}


//Does not insert into any variant
Instruction_t* copyInstruction(Instruction_t* instr)
{
	Instruction_t* cpy = new libIRDB::Instruction_t();

	copyInstruction(instr,cpy);

	return cpy;
}

Instruction_t* copyInstruction(FileIR_t* virp, Instruction_t* instr)
{
	Instruction_t* cpy = allocateNewInstruction(virp,instr);

	copyInstruction(instr,cpy);

	return cpy;
}

void copyInstruction(Instruction_t* src, Instruction_t* dest)
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

Instruction_t* allocateNewInstruction(FileIR_t* virp, DatabaseID_t p_fileID,Function_t* func)
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

Instruction_t* allocateNewInstruction(FileIR_t* virp, Instruction_t *template_instr)
{
	Function_t *func = template_instr->getFunction();
	DatabaseID_t fileID = template_instr->getAddress()->getFileID();
	return allocateNewInstruction(virp, fileID, func);
}

void IRDB_SDK::setInstructionAssembly(FileIR_t* virp,Instruction_t *p_instr, const string& p_assembly, Instruction_t *p_fallThrough, Instruction_t *p_target)
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

void IRDB_SDK::setInstructionAssembly(FileIR_t* virp,Instruction_t *p_instr, const string& p_assembly)
{
	IRDB_SDK::setInstructionAssembly(virp,p_instr,p_assembly, p_instr->getFallthrough(), p_instr->getTarget());
}


