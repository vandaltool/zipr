#include "Rewrite_Utility.hpp"

// Copied from PnTransform
// @todo: create a utility library with the one interface

using namespace std;
using namespace libIRDB;

using namespace IRDBUtility;

namespace IRDBUtility {
map<Function_t*, set<Instruction_t*> > inserted_instr; //used to undo inserted instructions
map<Function_t*, set<AddressID_t*> > inserted_addr; //used to undo inserted addresses


void setExitCode(FileIR_t* virp, Instruction_t* exit_code);

//For all insertBefore functions:
//The "first" instruction will have its contents replaced and a duplicate of "first" will be in the follow of first. 
//This duplicate is returned since the user already has a pointer to first.
//To insert before an instruction is the same as modifying the original instruction, and inserting after it
//a copy of the original instruction 
Instruction_t* insertAssemblyBefore(FileIR_t* virp, Instruction_t* first, string assembly, Instruction_t *target)
{
	Instruction_t* next = copyInstruction(virp,first);

	//In case the fallthrough is null, generate spri has to have a 
	//place to jump, which is determined by the original address.
	//This code is not placed in copyInstruction since this is only needed
	//when inserting before
	next->SetOriginalAddressID(first->GetOriginalAddressID());
	//"Null" out the original address (it should be as if the instruction was not in the database).
	first->SetOriginalAddressID(BaseObj_t::NOT_IN_DATABASE);
	first->GetRelocations().clear();

	virp->ChangeRegistryKey(first,next);
	setInstructionAssembly(virp,first,assembly,next,target);

	return next;
}

Instruction_t* insertAssemblyBefore(FileIR_t* virp, Instruction_t* first, string assembly)
{
	return insertAssemblyBefore(virp,first,assembly,NULL);
}

//Does not insert into any variant
Instruction_t* copyInstruction(Instruction_t* instr)
{
	Instruction_t* cpy = new Instruction_t();

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
	dest->SetDataBits(src->GetDataBits());
	dest->SetComment(src->GetComment());
	dest->SetCallback(src->GetCallback());
	dest->SetFallthrough(src->GetFallthrough());
	dest->SetTarget(src->GetTarget());
	dest->GetRelocations()=src->GetRelocations();
}

Instruction_t* allocateNewInstruction(FileIR_t* virp, db_id_t p_fileID,Function_t* func)
{
	Instruction_t *instr = new Instruction_t();
	AddressID_t *a =new AddressID_t();

	a->SetFileID(p_fileID);

	instr->SetFunction(func);
	instr->SetAddress(a);

	virp->GetInstructions().insert(instr);
	virp->GetAddresses().insert(a);

	inserted_instr[func].insert(instr);
	inserted_addr[func].insert(a);
	
	return instr;
}

Instruction_t* allocateNewInstruction(FileIR_t* virp, Instruction_t *template_instr)
{
	Function_t *func = template_instr->GetFunction();
	db_id_t fileID = template_instr->GetAddress()->GetFileID();
	return allocateNewInstruction(virp, fileID, func);
}

void setInstructionAssembly(FileIR_t* virp,Instruction_t *p_instr, string p_assembly, Instruction_t *p_fallThrough, Instruction_t *p_target)
{
	if (p_instr == NULL) return;
	
	///TODO: what if bad assembly?
	virp->RegisterAssembly(p_instr,p_assembly);

//	  p_instr->Assemble(p_assembly);
	p_instr->SetComment(p_instr->getDisassembly());
	p_instr->SetFallthrough(p_fallThrough); 
	p_instr->SetTarget(p_target); 
	
	virp->GetAddresses().insert(p_instr->GetAddress());
	virp->GetInstructions().insert(p_instr);
}

}
