

#include <libIRDB.hpp>
#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "beaengine/BeaEngine.h"

using namespace libIRDB;
using namespace std;

//
// returns true iff instruction is MUL or IMUL
//
bool isMultiplyInstruction(Instruction_t *p_instruction)
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

main(int argc, char* argv[])
{

	if(argc!=2)
	{
		cerr<<"Usage: integerbug <variant_id>"<<endl;
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

	cout<<"Do stuff to variant "<<*pidp<< "." <<endl;

	// For now, the overflow handler will consist of a HLT instruction
//	Instruction_t* overflowHandler = addOverflowHandler(virp);

        int numberMul = 0;
//	set<AddressID_t*> newaddressset;
	for(
		set<Instruction_t*>::const_iterator it=virp->GetInstructions().begin();
		it!=virp->GetInstructions().end(); 
		++it
	   )
	{
		Instruction_t* insn=*it;
                
		if (isMultiplyInstruction(insn) && insn->GetFunction())
		{
			numberMul++;
			cout << "found MUL: address: " << insn->GetAddress()
	                     << " comment: " << insn->GetComment()
	                     << " in function: " << insn->GetFunction()->GetName() << endl;
			// for now, insert overflow check to all IMUL instructions
			// later, we'll want to be more judicious about where to insert overflow checks
//			addOverflowCheck(virp, insn, overflowHandler);
		}

		
/*
                size_t found = insn->GetComment().find(string("mul"));
                if (found != string::npos)
		{
			numberMul++;
                	string dataBits = insn->GetDataBits();
			cout << "found MUL: address: " << insn->GetAddress()
	                     << " comment: " << insn->GetComment()
	                     << " dataBits: " << insn->GetDataBits();

			if (insn->GetFunction())
			{
	                	cout << " in function: " << insn->GetFunction()->GetName() << endl;
			}
			else
	                	cout << " in function: unknown" << endl;
		
		}
*/


                //
                // look for IMUL or MUL
                // then do something (initially just count them)
		//      define new instruction at some new address id:   <addressOfHalt> HLT 
		//               no targets, fallthrough is itself
                
		//      insert jo <addressOfhalt>      TARGET = <addressOfHalt> "jump overflow"
		//      fallthrough is what the old fallthrough would have been 
                //

/*
		AddressID_t *newaddr=new AddressID_t;
		newaddr->SetFileID(insn->GetAddress()->GetFileID());
		insn->SetAddress(newaddr);
		newaddressset.insert(newaddr);
*/
	}

//	virp->GetAddresses()=newaddressset;

//	cout<<"Writing variant "<<*pidp<<" back to database." << endl;
//	virp->WriteToDB();

	cout<<"Found " << numberMul << " MUL or IMUL instructions" << endl;
	pqxx_interface.Commit();
	cout<<"Done!"<<endl;

	delete pidp;
	delete virp;
}
