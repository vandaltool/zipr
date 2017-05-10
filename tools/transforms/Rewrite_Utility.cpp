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
using namespace std;
using namespace libIRDB;

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
	first->SetIBTargets(NULL);

	virp->ChangeRegistryKey(first,next);
	setInstructionAssembly(virp,first,assembly,next,target);

	return next;
}

Instruction_t* insertAssemblyBefore(FileIR_t* virp, Instruction_t* first, string assembly)
{
	return insertAssemblyBefore(virp,first,assembly,NULL);
}

Instruction_t* insertDataBitsBefore(FileIR_t* virp, Instruction_t* first, string dataBits)
{
	return insertDataBitsBefore(virp,first,dataBits,NULL);
}

Instruction_t* insertDataBitsBefore(FileIR_t* virp, Instruction_t* first, string dataBits, Instruction_t *target)
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

	setInstructionDataBits(virp,first,dataBits,next,target);

	return next;	
}

Instruction_t* insertAssemblyAfter(FileIR_t* virp, Instruction_t* first, string assembly, Instruction_t *target)
{
	Instruction_t *new_instr = allocateNewInstruction(virp,first);
	setInstructionAssembly(virp,new_instr,assembly,first->GetFallthrough(), target);
	first->SetFallthrough(new_instr);
	return new_instr;
}

Instruction_t* insertAssemblyAfter(FileIR_t* virp, Instruction_t* first, string assembly)
{
	return insertAssemblyAfter(virp,first,assembly,NULL);

}

Instruction_t* insertDataBitsAfter(FileIR_t* virp, Instruction_t* first, string dataBits, Instruction_t *target)
{
	Instruction_t *new_instr = allocateNewInstruction(virp,first);
	setInstructionDataBits(virp,new_instr,dataBits,first->GetFallthrough(), target);
	first->SetFallthrough(new_instr);

	return new_instr;
}

Instruction_t* insertDataBitsAfter(FileIR_t* virp, Instruction_t* first, string dataBits)
{
	return insertDataBitsAfter(virp,first,dataBits,NULL);
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
	dest->SetIBTargets(src->GetIBTargets()); 
	dest->GetRelocations()=src->GetRelocations();
	dest->SetEhProgram(src->GetEhProgram());
	dest->SetEhCallSite(src->GetEhCallSite());
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

Instruction_t* addNewAssembly(FileIR_t* firp, Instruction_t *p_instr, string p_asm)
{
        Instruction_t* newinstr;
        if (p_instr)
                newinstr = allocateNewInstruction(firp,p_instr->GetAddress()->GetFileID(), p_instr->GetFunction());
        else
                newinstr = allocateNewInstruction(firp,BaseObj_t::NOT_IN_DATABASE, NULL);

        firp->RegisterAssembly(newinstr, p_asm);

        if (p_instr)
        {
                newinstr->SetFallthrough(p_instr->GetFallthrough());
                p_instr->SetFallthrough(newinstr);
        }

        return newinstr;
}

Instruction_t* addNewDatabits(FileIR_t* firp, Instruction_t *p_instr, string p_bits)
{
        Instruction_t* newinstr;
        if (p_instr)
                newinstr = allocateNewInstruction(firp,p_instr->GetAddress()->GetFileID(), p_instr->GetFunction());
        else
                newinstr = allocateNewInstruction(firp,BaseObj_t::NOT_IN_DATABASE, NULL);

        newinstr->SetDataBits(p_bits);

        if (p_instr)
        {
                newinstr->SetFallthrough(p_instr->GetFallthrough());
                p_instr->SetFallthrough(newinstr);
        }

        return newinstr;
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

void setInstructionDataBits(FileIR_t* virp, Instruction_t *p_instr, string p_dataBits, Instruction_t *p_fallThrough, Instruction_t *p_target)
{
	if (p_instr == NULL) return;
	
	p_instr->SetDataBits(p_dataBits);
	p_instr->SetComment(p_instr->getDisassembly());
	p_instr->SetFallthrough(p_fallThrough); 
	p_instr->SetTarget(p_target); 
	
	virp->GetAddresses().insert(p_instr->GetAddress());
	virp->GetInstructions().insert(p_instr);
}

string getRetDataBits()
{
	string dataBits;
	dataBits.resize(1);
	dataBits[0] = 0xc3;
	return dataBits;
}

string getJumpDataBits()
{
	string dataBits;
	dataBits.resize(5);
	dataBits[0] = 0xe9;
	dataBits[1] = 0x00; // value doesn't matter -- we will fill it in later
	dataBits[2] = 0x00; // value doesn't matter -- we will fill it in later
	dataBits[3] = 0x00; // value doesn't matter -- we will fill it in later
	dataBits[4] = 0x00; // value doesn't matter -- we will fill it in later
	return dataBits;
}

// jns - jump not signed
string getJnsDataBits()
{
	string dataBits;
	dataBits.resize(2);
	dataBits[0] = 0x79;
	dataBits[1] = 0x00; // value doesn't matter -- we will fill it in later
	return dataBits;
}

// jz - jump zero
string	getJzDataBits()
{
	string dataBits;
	dataBits.resize(2);
	dataBits[0] = 0x74;
	dataBits[1] = 0x00; // value doesn't matter -- we will fill it in later

	return dataBits;
}

// jnz - jump not zero
string getJnzDataBits()
{
	string dataBits;
	dataBits.resize(2);
	dataBits[0] = 0x75;
	dataBits[1] = 0x00; // value doesn't matter -- we will fill it in later

	return dataBits;	
}

// jecxz - jump ecx zero
string getJecxzDataBits()
{
	string dataBits;
	dataBits.resize(2);
	dataBits[0] = 0xe3;
	dataBits[1] = 0x00; // value doesn't matter -- we will fill it in later

	return dataBits;	
}

Instruction_t* getHandlerCode(FileIR_t* virp, Instruction_t* fallthrough, mitigation_policy policy)
{
	Instruction_t *handler_code ;
	if(virp->GetArchitectureBitWidth()==32)
	{
#ifdef CGC
		handler_code = allocateNewInstruction(virp,fallthrough);
		setInstructionAssembly(virp,handler_code,"mov eax, 1",NULL,NULL);
		Instruction_t* int80 = insertAssemblyAfter(virp,handler_code,"int 0x80",NULL);
		int80->SetFallthrough(fallthrough);
#else
		handler_code = allocateNewInstruction(virp,fallthrough);
		setInstructionAssembly(virp,handler_code,"pusha",NULL,NULL);
		Instruction_t* pushf = insertAssemblyAfter(virp,handler_code,"pushf",NULL);
		stringstream ss;
		ss<<"push dword 0x";
		ss<<hex<<policy;
		Instruction_t *policy_push = insertAssemblyAfter(virp,pushf,ss.str(),NULL);
		ss.str("");
		ss<<"push dword 0x";
		ss<<hex<<fallthrough->GetAddress()->GetVirtualOffset();
		Instruction_t *addr_push = insertAssemblyAfter(virp,policy_push,ss.str(),NULL);
		//I am not planning on returning, but pass the address at which the overflow was detected.
		Instruction_t *ret_push = insertAssemblyAfter(virp,addr_push,ss.str(),NULL);
		Instruction_t *callback = insertAssemblyAfter(virp,ret_push,"nop",NULL);
	
		callback->SetCallback("buffer_overflow_detector");
		
	
		Instruction_t *popf = insertAssemblyAfter(virp,callback,"popf",NULL);
		Instruction_t *popa = insertAssemblyAfter(virp,popf,"popa",NULL);
		popa->SetFallthrough(fallthrough);
#endif
		
	}
	else
	{
		assert(virp->GetArchitectureBitWidth()==64);
		if (policy == P_CONTROLLED_EXIT) 
		{
			handler_code = allocateNewInstruction(virp,fallthrough);
			setInstructionAssembly(virp,handler_code,"mov rdi, 189",NULL,NULL);
			Instruction_t* syscall_num = insertAssemblyAfter(virp,handler_code,"mov rax, 60",NULL);
			Instruction_t* syscall_i = insertAssemblyAfter(virp,syscall_num,"syscall",NULL);
			syscall_i->SetFallthrough(fallthrough);
		}
		else
		{
			handler_code= allocateNewInstruction(virp,fallthrough);
			setInstructionAssembly(virp,handler_code,"hlt",NULL,NULL);
			handler_code->SetComment("hlt ; Make this into a callback: jdh@getHandlerCode");
			handler_code->SetFallthrough(fallthrough);
		}
	}

	return handler_code;
}

Instruction_t* insertCanaryCheckBefore(FileIR_t* virp,Instruction_t *first, unsigned int canary_val, int esp_offset, Instruction_t *fail_code)
{
	auto do_zero=(first->getDisassembly().find("ret")!=string::npos);
	stringstream ss;
	const char *sp_reg="esp";
	if(virp->GetArchitectureBitWidth()==64)
		sp_reg="rsp";

	ss<<"cmp dword ["<<sp_reg;

	bool esp_neg=false;
	if(esp_offset <0)
	{
		ss<<"-";
		esp_offset = esp_offset*-1;
		esp_neg=true;
	}
	else
		ss<<"+";

	ss<<"0x"<<hex<<esp_offset<<"], 0x"<<hex<<canary_val;

	//Insert the cmp before 
	Instruction_t* next = insertAssemblyBefore(virp,first,ss.str());

	//Then insert the jmp after the compare. 
	//The fallthrough of the inserted jmp will be a copy of the original
	//instruction, still pointed to by "first".
	insertDataBitsAfter(virp,first,getJnzDataBits(),fail_code);
	first->SetComment("Canary Check: "+first->GetComment());

	//TODO: move canary zero to option 
	if(esp_neg)
		esp_offset *= -1;

	if(do_zero)
		insertCanaryZeroAfter(virp,first,esp_offset,fail_code); 

	return next;

}

Instruction_t* insertCanaryZeroAfter(FileIR_t* virp, Instruction_t *first, int esp_offset, Instruction_t *fail_code)
{
	stringstream ss;
	const char *sp_reg="esp";
        if(virp->GetArchitectureBitWidth()==64)
	{
                sp_reg="rsp";
        	ss<<"mov qword ["<<sp_reg; // clear all 64-bits
	}
	else
	{
        	ss<<"mov dword ["<<sp_reg;
	}

        if(esp_offset <0)
        {
                ss<<"-";
                esp_offset = esp_offset*-1;
        }
        else
                ss<<"+";

        ss<<"0x"<<hex<<esp_offset<<"], 0x0";

        //Insert the cmp before 
        Instruction_t* next = insertAssemblyAfter(virp,first,ss.str());
        first->SetComment("Canary Zero: "+first->GetComment());

        return next;
}

Relocation_t* createNewRelocation(FileIR_t* firp, Instruction_t* insn, string type, int offset)
{
        Relocation_t* reloc=new Relocation_t;
        insn->GetRelocations().insert(reloc);
        firp->GetRelocations().insert(reloc);

	reloc->SetType(type);
	reloc->SetOffset(offset);

        return reloc;
}

