

#include <libIRDB.hpp>
#include <iostream>
#include <stdlib.h>
#include "beaengine/BeaEngine.h"
#include <assert.h>
#include <string.h>



using namespace libIRDB;
using namespace std;


void fix_call(Instruction_t* insn, VariantIR_t *virp)
{

        DISASM disasm;
        memset(&disasm, 0, sizeof(DISASM));

        disasm.Options = NasmSyntax + PrefixedNumeral;
        disasm.Archi = 32;
        disasm.EIP = (UIntPtr)insn->GetDataBits().c_str();
        disasm.VirtualAddr = insn->GetAddress()->GetVirtualOffset();

        /* Disassemble the instruction */
        int instr_len = Disasm(&disasm);


	/* if this instruction is an inserted call instruction and we don't need to 
	 * convert it for correctness' sake.
	 */
	if(insn->GetAddress()->GetVirtualOffset()==0)
		return;

	virtual_offset_t next_addr=insn->GetAddress()->GetVirtualOffset() + insn->GetDataBits().length();

	/* create a new instruction and a new addresss for it that do not correspond to any original program address */
	Instruction_t *callinsn=new Instruction_t();
	AddressID_t *calladdr=new AddressID_t;
       	calladdr->SetFileID(insn->GetAddress()->GetFileID());
	insn->SetAddress(calladdr);

	/* set the fields in the new instruction */
	callinsn->SetAddress(calladdr);
	callinsn->SetTarget(insn->GetTarget());
	callinsn->SetFallthrough(insn->GetFallthrough());
	callinsn->SetFunction(insn->GetFunction());
	callinsn->SetComment(insn->GetComment()+" Jump part");

	/* set the new instruction's data bits to be a jmp instead of a call */
	string newbits=insn->GetDataBits();
	switch((unsigned char)newbits[0])
	{
		case 0xff:
			if(((((unsigned char)newbits[1])&0x38)>>3) == 2)
			{
				newbits[1]&=0xC7;	// remove old bits
				newbits[1]|=(0x4<<3);	// set r/m field to 4
			}
			else if(((((unsigned char)newbits[1])&0x38)>>3) == 3)
			{
				newbits[1]&=0xC7;	// remove old bits
				newbits[1]|=(0x5<<3);	// set r/m field to 5
			}
			else
				assert(0);
			break;
		case 0x9A:
			newbits[0]=0xEA;
			break;
		case 0xE8:
			newbits[0]=0xE9;
			break;
		default:
			assert(0);
	}
	callinsn->SetDataBits(newbits);

	/* add the new insn and new address into the list of valid calls and addresses */
	virp->GetAddresses().insert(calladdr);
	virp->GetInstructions().insert(callinsn);

	/* Convert the old call instruction into a push return_address instruction */
	insn->SetFallthrough(callinsn);
	insn->SetTarget(NULL);
	newbits=string("");
	newbits.resize(5);
	newbits[0]=0x68;	/* assemble an instruction push next_addr */
	newbits[1]=(next_addr>>0) & 0xff;
	newbits[2]=(next_addr>>8) & 0xff;
	newbits[3]=(next_addr>>16) & 0xff;
	newbits[4]=(next_addr>>24) & 0xff;
	insn->SetDataBits(newbits);
	insn->SetComment(insn->GetComment()+" Push part");
	
}


//
// return true if insn is a call
//
bool is_call(Instruction_t* insn)
{

        DISASM disasm;
        memset(&disasm, 0, sizeof(DISASM));

        disasm.Options = NasmSyntax + PrefixedNumeral;
        disasm.Archi = 32;
        disasm.EIP = (UIntPtr)insn->GetDataBits().c_str();
        disasm.VirtualAddr = insn->GetAddress()->GetVirtualOffset();

        /* Disassemble the instruction */
        int instr_len = Disasm(&disasm);
	

	return (disasm.Instruction.BranchType==CallType);
}


//
// main rountine; convert calls into push/jump statements 
//
main(int argc, char* argv[])
{

	if(argc!=2)
	{
		cerr<<"Usage: ilr <id>"<<endl;
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

	cout<<"Fixing calls->push/jmp in variant "<<*pidp<< "." <<endl;

	for(
		set<Instruction_t*>::const_iterator it=virp->GetInstructions().begin();
		it!=virp->GetInstructions().end(); 
		++it
	   )
	{

		Instruction_t* insn=*it;
		if(is_call(insn))
			fix_call(insn, virp);
	}

	cout<<"Writing variant "<<*pidp<<" back to database." << endl;
	virp->WriteToDB();

	pqxx_interface.Commit();
	cout<<"Done!"<<endl;

	delete pidp;
	delete virp;
}
