

#include <libIRDB.hpp>
#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "beaengine/BeaEngine.h"

using namespace libIRDB;
using namespace std;


//
// return available offset 
//
virtual_offset_t getAvailableAddress(VariantIR_t *p_virp)
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
string getAssembly(Instruction_t *p_instruction)
{
	DISASM disasm;
	memset(&disasm, 0, sizeof(DISASM));

	disasm.Archi = 32;
	disasm.EIP = (UIntPtr) p_instruction->GetDataBits().c_str();
	disasm.VirtualAddr = p_instruction->GetAddress()->GetVirtualOffset();

	Disasm(&disasm); // dissassemble the instruction

	return string(disasm.CompleteInstr);
}

//
// Returns true iff instruction is mul or imul
//
bool isMultiplyInstruction32(Instruction_t *p_instruction)
{
	if (!p_instruction)
		return false;

	DISASM disasm;
	memset(&disasm, 0, sizeof(DISASM));

	disasm.Options = NasmSyntax + PrefixedNumeral;
	disasm.Archi = 32;
	disasm.EIP = (UIntPtr) p_instruction->GetDataBits().c_str();
	disasm.VirtualAddr = p_instruction->GetAddress()->GetVirtualOffset();

	Disasm(&disasm); // dissassemble the instruction

	// look for "mul ..." or "imul ..."
	string disassembly = string(disasm.CompleteInstr);
	size_t found_pos = disassembly.find("mul");
        return (found_pos == 0 || found_pos == 1); // we expect mul or imul to be the first word in the string
}

//
//      jno <originalFallthroughInstruction>
//      pusha
//      pushf
//      push L1
//      ... setup detector ...
//  L1: popf
//      popa
//
void addOverflowCheck(VariantIR_t *p_virp, Instruction_t *p_instruction)
{
	assert(p_virp && p_instruction);
	
        string dataBits;

	AddressID_t *jno_a =new AddressID_t;
	AddressID_t *pusha_a =new AddressID_t;
	AddressID_t *pushf_a =new AddressID_t;
	AddressID_t *pusharg_a =new AddressID_t;
	AddressID_t *pushret_a =new AddressID_t;
	AddressID_t *popf_a =new AddressID_t;
	AddressID_t *poparg_a =new AddressID_t;
	AddressID_t *popa_a =new AddressID_t;

	jno_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	pusha_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	pushf_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	pusharg_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	pushret_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	popf_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	poparg_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	popa_a->SetFileID(p_instruction->GetAddress()->GetFileID());

	Instruction_t* jno_i = new Instruction_t;
	Instruction_t* pusha_i = new Instruction_t;
	Instruction_t* pushf_i = new Instruction_t;
	Instruction_t* pusharg_i = new Instruction_t;
	Instruction_t* pushret_i = new Instruction_t;
	Instruction_t* popf_i = new Instruction_t;
	Instruction_t* poparg_i = new Instruction_t;
	Instruction_t* popa_i = new Instruction_t;

	// pin the popf instruction to a free address
	virtual_offset_t postDetectorReturn = getAvailableAddress(p_virp);
fprintf(stderr,"post detector return set to: 0x%x\n", postDetectorReturn);
	popf_a->SetVirtualOffset(postDetectorReturn);

        jno_i->SetAddress(jno_a);
        pusha_i->SetAddress(pusha_a);
        pushf_i->SetAddress(pushf_a);
        pusharg_i->SetAddress(pusharg_a);
        pushret_i->SetAddress(pushret_a);
        popf_i->SetAddress(popf_a);
        poparg_i->SetAddress(poparg_a);
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
        poparg_i->SetComment(getAssembly(poparg_i) + " -- with callback to integer_overflow_detector()");
	poparg_i->SetFallthrough(popf_i); 
	poparg_i->SetIsIndirectTarget(true);  

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

	fprintf(stderr,"sanity check address of popf: 0x%x %u   fileID:%d\n", popf_a->GetVirtualOffset(), popf_a->GetVirtualOffset(), popf_a->GetFileID());
	fprintf(stderr,"sanity check address of pushf: 0x%x %u   fileID:%d\n", pushf_a->GetVirtualOffset(), pushf_a->GetVirtualOffset(), pushf_a->GetFileID());

}

main(int argc, char* argv[])
{

	if(argc!=2)
	{
		cerr<<"Usage: integerbugtransform.exe <variant_id>"<<endl;
		exit(-1);
	}

	VariantID_t *pidp=NULL;
	VariantIR_t *virp=NULL;

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

        int numberMul = 0;
	for(
		set<Instruction_t*>::const_iterator it=virp->GetInstructions().begin();
		it!=virp->GetInstructions().end(); 
		++it
	   )
	{
		Instruction_t* insn=*it;
                
		if (isMultiplyInstruction32(insn) && insn->GetFunction() 
		/*	&& insn->GetFunction()->GetName().find("test_") != string::npos */ )
		{
			numberMul++;
			cout << "found MUL: address: " << insn->GetAddress()
	                     << " comment: " << insn->GetComment()
	                     << " in function: " << insn->GetFunction()->GetName() << endl;
			// for now, insert overflow check to all IMUL instructions
			// later, we'll want to be more judicious about where to insert overflow checks
			addOverflowCheck(virp, insn);
		}
	}


	cout<<"Writing variant "<<*pidp<<" back to database." << endl;
	virp->WriteToDB();

	cout<<"Found " << numberMul << " MUL or IMUL instructions" << endl;
	pqxx_interface.Commit();
	cout<<"Done!"<<endl;

	delete pidp;
	delete virp;
}
