

#include <libIRDB.hpp>
#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "beaengine/BeaEngine.h"

using namespace libIRDB;
using namespace std;

string getAssembly(Instruction_t *p_instruction)
{
	DISASM disasm;
	memset(&disasm, 0, sizeof(DISASM));

	disasm.Options = NasmSyntax + PrefixedNumeral;
	disasm.Archi = 32;
	disasm.EIP = (UIntPtr) p_instruction->GetDataBits().c_str();
	disasm.VirtualAddr = p_instruction->GetAddress()->GetVirtualOffset();

	Disasm(&disasm); // dissassemble the instruction

	return string(disasm.CompleteInstr);
}

//
// returns true iff instruction is mul or imul
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

Instruction_t* addOverflowHandler(VariantIR_t *p_virp)
{
	AddressID_t *newaddr=new AddressID_t;
	Instruction_t* hltInstruction = new Instruction_t();

        string haltDataBits;

        haltDataBits.resize(1);
        haltDataBits[0] = 0xF4;

	hltInstruction->SetComment("hlt"); // 0xF4
	hltInstruction->SetDataBits(haltDataBits);
	hltInstruction->SetFallthrough(hltInstruction); // yes, set to self
	hltInstruction->SetAddress(newaddr);

	p_virp->GetAddresses().insert(newaddr);
	p_virp->GetInstructions().insert(hltInstruction);
        cout << "Sanity check for hlt: " << getAssembly(hltInstruction) << endl;
	return hltInstruction;
}

void addOverflowCheck(VariantIR_t *p_virp, Instruction_t *p_instruction, Instruction_t *p_overflowHandler)
{
	// insert after the instruction, a check for overflow:
	//       imul ...
	//       jo <overflowHandler> 
	//       <instr...>
	//       <instr...>

	AddressID_t *newaddr=new AddressID_t;
	Instruction_t* overflowCheckInstruction = new Instruction_t();
        string overflowCheckDataBits;

        overflowCheckDataBits.resize(2);
        overflowCheckDataBits[0] = 0x70;
        overflowCheckDataBits[1] = 0x0; // we really don't care here as the SPASM generator will use the target address and fill this in correctly
	overflowCheckInstruction->SetDataBits(overflowCheckDataBits);
	overflowCheckInstruction->SetComment("jo <addressOverflowHandler>");       // 0x70
	overflowCheckInstruction->SetTarget(p_overflowHandler);                    // jump overflow target = overflow handler
	overflowCheckInstruction->SetFallthrough(p_instruction->GetFallthrough()); // use the original fallthrough
	overflowCheckInstruction->SetAddress(newaddr);

	// do I have to do anything with the file ID?

	p_instruction->SetFallthrough(overflowCheckInstruction);

	p_virp->GetAddresses().insert(newaddr);
	p_virp->GetInstructions().insert(overflowCheckInstruction);

        cout << "Sanity check for jo: " << getAssembly(overflowCheckInstruction) << endl;
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

	// For now, the overflow handler will consist of a HLT instruction
	Instruction_t* overflowHandler = addOverflowHandler(virp);

        int numberMul = 0;
	for(
		set<Instruction_t*>::const_iterator it=virp->GetInstructions().begin();
		it!=virp->GetInstructions().end(); 
		++it
	   )
	{
		Instruction_t* insn=*it;
                
		if (isMultiplyInstruction32(insn) && insn->GetFunction() 
		/*	&& insn->GetFunction()->GetName().find("test_") != string::npos */)
		{
			numberMul++;
			cout << "found MUL: address: " << insn->GetAddress()
	                     << " comment: " << insn->GetComment()
	                     << " in function: " << insn->GetFunction()->GetName() << endl;
			// for now, insert overflow check to all IMUL instructions
			// later, we'll want to be more judicious about where to insert overflow checks
			addOverflowCheck(virp, insn, overflowHandler);
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
