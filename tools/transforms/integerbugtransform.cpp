

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

void addOverflowCheck(VariantIR_t *p_virp, Instruction_t *p_instruction/*, Instruction_t *p_overflowHandler*/)
{
/*
insert after the instruction, a check for overflow:
     imul ...
     jno I0
     pusha
     pushf
     call L0
L0:  add dword[esp], 8     ; 8 is 7 (size of this instruction) + 1 (size of the jmp)
     jmp detector     ; this will be done by strata, we need to emit . () detector
     popf
     popa
I0:  <original next instruction>

From spasm, to get the right hex values:

ff000000 ** 2 71 15     #jno I0
ff000002 ** 1 60        #pusha
ff000003 ** 1 9c        #pushf
ff000004 ** 5 e8 00 00 00 00    #call L0
FF000009 ** 7 81 04 24 08 00 00 00      #src addr = <L0> ; add dword [esp], 0x08
ff000015 ** 1 9d        #popf + callback detector (reserved 1 byte for strata's jmp/callback
ff000016 ** 1 61        #popa
FF000017 ** 1 f4        #src addr = <I0> ; hlt
FF000018 ** 1 90        #src addr = <foobar> ; nop
*/

        string dataBits;

	AddressID_t *jno_a =new AddressID_t;
	AddressID_t *pusha_a =new AddressID_t;
	AddressID_t *pushf_a =new AddressID_t;
	AddressID_t *pusharg1_a =new AddressID_t;
	AddressID_t *call_a =new AddressID_t;
	AddressID_t *add_a =new AddressID_t;
	AddressID_t *popf_a =new AddressID_t;
	AddressID_t *popa_a =new AddressID_t;

	Instruction_t* jno_i = new Instruction_t();
	Instruction_t* pusha_i = new Instruction_t();
	Instruction_t* pushf_i = new Instruction_t();
	Instruction_t* pusharg1_i = new Instruction_t();
	Instruction_t* call_i = new Instruction_t();
	Instruction_t* add_i = new Instruction_t();
	Instruction_t* popf_i = new Instruction_t();
	Instruction_t* popa_i = new Instruction_t();


        jno_i->SetAddress(jno_a);
        pusha_i->SetAddress(pusha_a);
        pushf_i->SetAddress(pushf_a);
        call_i->SetAddress(call_a);
        add_i->SetAddress(add_a);
        popf_i->SetAddress(popf_a);
        popa_i->SetAddress(popa_a);

        // add new address to IR
	p_virp->GetAddresses().insert(jno_a);
	p_virp->GetAddresses().insert(pusha_a);
	p_virp->GetAddresses().insert(pushf_a);
	p_virp->GetAddresses().insert(call_a);
	p_virp->GetAddresses().insert(add_a);
	p_virp->GetAddresses().insert(popf_a);
	p_virp->GetAddresses().insert(popa_a);

        // add new instructions to IR
	p_virp->GetInstructions().insert(jno_i);
	p_virp->GetInstructions().insert(pusha_i);
	p_virp->GetInstructions().insert(pushf_i);
	p_virp->GetInstructions().insert(call_i);
	p_virp->GetInstructions().insert(add_i);
	p_virp->GetInstructions().insert(popf_i);
	p_virp->GetInstructions().insert(popa_i);

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
	pusha_i->SetFallthrough(pushf_i); 

        // pushf   
        dataBits.resize(1);
        dataBits[0] = 0x9c;
        pushf_i->SetDataBits(dataBits);
        pushf_i->SetComment(getAssembly(pushf_i));
	pushf_i->SetFallthrough(call_i); 

        // call ff000004 ** 5 e8 00 00 00 00    #call L0
        dataBits.resize(5);
        dataBits[0] = 0xe8;
        dataBits[1] = 0x01;
        dataBits[2] = 0x00;
        dataBits[3] = 0x00;
        dataBits[4] = 0x00;
        call_i->SetDataBits(dataBits);
        call_i->SetTarget(add_i);
        call_i->SetFallthrough(add_i);
        call_i->SetComment(getAssembly(call_i));

        // add FF000009 ** 8 36 81 04 24 09 00 00 00      #src addr = <L0> ; add dword [esp], 0x0A
        dataBits.resize(8);
        dataBits[0] = 0x36;
        dataBits[1] = 0x81;
        dataBits[2] = 0x04;
        dataBits[3] = 0x24;
        dataBits[4] = 0x0A; // 8 + 1 + 1?
        dataBits[5] = 0x00;
        dataBits[6] = 0x00;
        dataBits[7] = 0x00;
        add_i->SetDataBits(dataBits);
        add_i->SetFallthrough(popf_i);
        add_i->SetComment(getAssembly(add_i));
        cout << "Sanity check for add: " << getAssembly(add_i) << endl;

        // popf   
        dataBits.resize(1);
        dataBits[0] = 0x9d;
        popf_i->SetDataBits(dataBits);
        popf_i->SetComment(getAssembly(popf_i) + string(" -- with callback to test_detector()"));
	popf_i->SetFallthrough(popa_i); 
	popf_i->SetCallback("test_detector"); 

        // popa   
        dataBits.resize(1);
        dataBits[0] = 0x61;
        popa_i->SetDataBits(dataBits);
        popa_i->SetComment(getAssembly(popa_i));
	popa_i->SetFallthrough(nextOrig_i); 
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
			&& insn->GetFunction()->GetName().find("test_") != string::npos )
		{
			numberMul++;
			cout << "found MUL: address: " << insn->GetAddress()
	                     << " comment: " << insn->GetComment()
	                     << " in function: " << insn->GetFunction()->GetName() << endl;
			// for now, insert overflow check to all IMUL instructions
			// later, we'll want to be more judicious about where to insert overflow checks
//			addOverflowCheck(virp, insn, overflowHandler);
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
