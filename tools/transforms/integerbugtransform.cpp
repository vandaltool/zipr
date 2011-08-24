#include <libIRDB.hpp>
#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "beaengine/BeaEngine.h"

#include "transformutils.h"

using namespace libIRDB;
using namespace std;

//
// todo:
//       getAssembly is now part of the interface for an instruction -- reuse if needed
//       accept white list
//       move utility functions to instruction interface
//

//
// return available offset 
//
static virtual_offset_t getAvailableAddress(VariantIR_t *p_virp)
{
	// traverse all instructions
	// grab address

	virtual_offset_t availableAddressOffset = 0;
	for(
		set<Instruction_t*>::const_iterator it=p_virp->GetInstructions().begin();
		it!=p_virp->GetInstructions().end(); 
		++it
	   )
	{
		Instruction_t* insn=*it;
		if (!insn) continue;

		AddressID_t* addr = insn->GetAddress();
		virtual_offset_t offset = addr->GetVirtualOffset();
                
		if (offset > availableAddressOffset)
		{
			availableAddressOffset = offset;
		}
	}

	// @todo: lookup instruction size so that we don't waste any space
	// for some reason the max available address is incorrect! was ist los?
static int counter = -16;
        counter += 16;
	return 0xf0000000 + counter;
// availableAddressOffset + 16;
}

//
// Given an instruction, returns its disassembly
//
static string getAssembly(Instruction_t *p_instruction)
{
	DISASM disasm;

	p_instruction->Disassemble(disasm);
	return string(disasm.CompleteInstr);
}

//
// Returns true iff instruction is mul or imul
//
static bool isMultiplyInstruction32(Instruction_t *p_instruction)
{
	if (!p_instruction)
		return false;

	DISASM disasm;

	p_instruction->Disassemble(disasm);

	// look for "mul ..." or "imul ..."
	// beaengine adds space at the end of the mnemonic string
	return strcasestr(disasm.Instruction.Mnemonic, "mul ") != NULL;
}

//
// Returns true iff instruction is mul or imul
//
static bool isAddSubNonEspInstruction32(Instruction_t *p_instruction)
{
	if (!p_instruction)
		return false;

	DISASM disasm;

	// look for "add ..." or "sub ..."
	p_instruction->Disassemble(disasm);

	// beaengine adds space at the end of the mnemonic string
	if (strcasestr(disasm.Instruction.Mnemonic, "add "))
	{
		return true;
	}
	else if (strcasestr(disasm.Instruction.Mnemonic, "sub "))
	{
		if (strcasestr(disasm.Argument1.ArgMnemonic,"esp") &&
			(disasm.Argument2.ArgType & 0xFFFF0000 & (CONSTANT_TYPE | ABSOLUTE_)))
		{
			// optimization: filter out "sub esp, K"
			return false;
		}
		return true;
	}

	return false;
}

//
//      jno <originalFallthroughInstruction>
//      pusha
//      pushf
//      push_arg
//      push L1
//      ... setup detector ...
//  L1: pop_arg
//      popf
//      popa
//
static void addOverflowCheck(VariantIR_t *p_virp, Instruction_t *p_instruction, std::string p_detector)
{
	assert(p_virp && p_instruction);
	
        string dataBits;

	AddressID_t *jno_a =new AddressID_t;
	AddressID_t *pusha_a =new AddressID_t;
	AddressID_t *pushf_a =new AddressID_t;
	AddressID_t *pusharg_a =new AddressID_t;
	AddressID_t *pushret_a =new AddressID_t;
	AddressID_t *poparg_a =new AddressID_t;
	AddressID_t *popf_a =new AddressID_t;
	AddressID_t *popa_a =new AddressID_t;

	jno_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	pusha_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	pushf_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	pusharg_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	pushret_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	poparg_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	popf_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	popa_a->SetFileID(p_instruction->GetAddress()->GetFileID());

	Instruction_t* jno_i = new Instruction_t;
	Instruction_t* pusha_i = new Instruction_t;
	Instruction_t* pushf_i = new Instruction_t;
	Instruction_t* pusharg_i = new Instruction_t;
	Instruction_t* pushret_i = new Instruction_t;
	Instruction_t* poparg_i = new Instruction_t;
	Instruction_t* popf_i = new Instruction_t;
	Instruction_t* popa_i = new Instruction_t;

	// pin the poparg instruction 
	virtual_offset_t postDetectorReturn = getAvailableAddress(p_virp);
	poparg_a->SetVirtualOffset(postDetectorReturn);

        jno_i->SetAddress(jno_a);
        pusha_i->SetAddress(pusha_a);
        pushf_i->SetAddress(pushf_a);
        pusharg_i->SetAddress(pusharg_a);
        pushret_i->SetAddress(pushret_a);
        poparg_i->SetAddress(poparg_a);
        popf_i->SetAddress(popf_a);
        popa_i->SetAddress(popa_a);

        // handle the original mul or imul instruction
	Instruction_t* nextOrig_i = p_instruction->GetFallthrough();
        p_instruction->SetFallthrough(jno_i); 

        // jno IO
        dataBits.resize(2);
        dataBits[0] = 0x71;
        dataBits[1] = 0x15; // value doesn't matter, we will fill it in later
        jno_i->SetDataBits(dataBits);
        jno_i->SetComment(getAssembly(jno_i));
	jno_i->SetFallthrough(pusha_i); 
	jno_i->SetTarget(nextOrig_i); 

        // pusha   
        dataBits.resize(1);
        dataBits[0] = 0x60;
        pusha_i->SetDataBits(dataBits);
        pusha_i->SetComment(getAssembly(pusha_i));
	pusha_i->SetFallthrough(pusharg_i); 

        // pushf   
        dataBits.resize(1);
        dataBits[0] = 0x9c;
        pushf_i->SetDataBits(dataBits);
        pushf_i->SetComment(getAssembly(pushf_i));
	pushf_i->SetFallthrough(pusharg_i); 

        // push arg
        dataBits.resize(5);
        dataBits[0] = 0x68;
	virtual_offset_t *tmp = (virtual_offset_t *) &dataBits[1];
	*tmp = p_instruction->GetAddress()->GetVirtualOffset();
        pusharg_i->SetDataBits(dataBits);
        pusharg_i->SetComment(getAssembly(pusharg_i));
	pusharg_i->SetFallthrough(pushret_i); 

        // pushret   
        dataBits.resize(5);
        dataBits[0] = 0x68;
	tmp = (virtual_offset_t *) &dataBits[1];
	*tmp = postDetectorReturn;
        pushret_i->SetDataBits(dataBits);
        pushret_i->SetComment(getAssembly(pushret_i));
	pushret_i->SetFallthrough(poparg_i); 

	// poparg
        dataBits.resize(1);
        dataBits[0] = 0x58;
        poparg_i->SetDataBits(dataBits);
        poparg_i->SetComment(getAssembly(poparg_i) + " -- with callback to " + p_detector);
	poparg_i->SetFallthrough(popa_i); 
	poparg_i->SetIndirectBranchTargetAddress(poparg_a);  
	poparg_i->SetCallback(p_detector); 

        // popf   
        dataBits.resize(1);
        dataBits[0] = 0x9d;
        popf_i->SetDataBits(dataBits);
        popf_i->SetComment(getAssembly(popf_i));
	popf_i->SetFallthrough(popa_i); 
	popf_i->SetCallback("integer_overflow_detector"); 

        // popa   
        dataBits.resize(1);
        dataBits[0] = 0x61;
        popa_i->SetDataBits(dataBits);
        popa_i->SetComment(getAssembly(popa_i));
	popa_i->SetFallthrough(nextOrig_i); 

        // add new address to IR
	p_virp->GetAddresses().insert(jno_a);
	p_virp->GetAddresses().insert(pusha_a);
	p_virp->GetAddresses().insert(pusharg_a);
	p_virp->GetAddresses().insert(pushf_a);
	p_virp->GetAddresses().insert(pushret_a);
	p_virp->GetAddresses().insert(popf_a);
	p_virp->GetAddresses().insert(poparg_a);
	p_virp->GetAddresses().insert(popa_a);

        // add new instructions to IR
	p_virp->GetInstructions().insert(jno_i);
	p_virp->GetInstructions().insert(pusha_i);
	p_virp->GetInstructions().insert(pusharg_i);
	p_virp->GetInstructions().insert(pushf_i);
	p_virp->GetInstructions().insert(pushret_i);
	p_virp->GetInstructions().insert(popf_i);
	p_virp->GetInstructions().insert(poparg_i);
	p_virp->GetInstructions().insert(popa_i);
}

main(int argc, char* argv[])
{

	if(argc < 2)
	{
		cerr<<"Usage: integerbugtransform.exe <variant_id> <filtered_functions>"<<endl;
		exit(-1);
	}

	VariantID_t *pidp=NULL;
	VariantIR_t *virp=NULL;

	set<string> filteredFunctions; 

	if (argc == 3)
	{ 
		filteredFunctions = getFunctionList(argv[2]);

	}

	/* setup the interface to the sql server */
	pqxxDB_t pqxx_interface;
	BaseObj_t::SetInterface(&pqxx_interface);

	cout<<"Reading variant "<<string(argv[1])<<" from database." << endl;
	try 
	{

		pidp=new VariantID_t(atoi(argv[1]));

		assert(pidp->IsRegistered()==true);

		// read the db  
		virp=new VariantIR_t(*pidp);
	}
	catch (DatabaseError_t pnide)
	{
		cout<<"Unexpected database error: "<<pnide<<endl;
		exit(-1);
        }

	assert(virp && pidp);

        int numMul = 0, numAddSub = 0;
	for(
		set<Instruction_t*>::const_iterator it=virp->GetInstructions().begin();
		it!=virp->GetInstructions().end(); 
		++it
	   )
	{
		Instruction_t* insn=*it;
                
		if (!insn->GetFunction()) continue;

		cerr << "INTXFORM: looking at fn: " << insn->GetFunction()->GetName() << endl;
		if (filteredFunctions.find(insn->GetFunction()->GetName()) != filteredFunctions.end())
		{
			cerr << "INTXFORM: filtered out fn: " << insn->GetFunction()->GetName() << endl;
	                continue;
		}

		if (isMultiplyInstruction32(insn))
		{
			numMul++;
			cout << "found Mul: address: " << insn->GetAddress()
	                     << " comment: " << insn->GetComment()
	                     << " in function: " << insn->GetFunction()->GetName() << endl;
			// for now, insert overflow check to all IMUL instructions
			// later, we'll want to be more judicious about where to insert overflow checks
			addOverflowCheck(virp, insn, "mul_overflow_detector_32");
		}
		else if (isAddSubNonEspInstruction32(insn))
		{
			numAddSub++;
			cout << "found Add/Sub: address: " << insn->GetAddress()
	                     << " comment: " << insn->GetComment()
	                     << " in function: " << insn->GetFunction()->GetName() << endl;
			addOverflowCheck(virp, insn, "addsub_overflow_detector_32");
		}
	}

	cout<<"Writing variant "<<*pidp<<" back to database." << endl;
	virp->WriteToDB();

	cout<<"Found " << numMul << " Mul instructions" << endl;
	cout<<"Found " << numAddSub << " Add/Sub instructions" << endl;
	pqxx_interface.Commit();
	cout<<"Done!"<<endl;

	delete pidp;
	delete virp;
}
