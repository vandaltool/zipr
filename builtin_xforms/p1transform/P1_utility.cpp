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

#include "P1_utility.hpp"
using namespace std;
using namespace IRDB_SDK;

map<Function_t*, set<Instruction_t*> > inserted_instr; //used to undo inserted instructions
map<Function_t*, set<AddressID_t*> > inserted_addr; //used to undo inserted addresses


void setExitCode(FileIR_t* virp, Instruction_t* exit_code);


Instruction_t* P1_insertAssemblyBefore(FileIR_t* virp, Instruction_t* first, string assembly, Instruction_t *target)
{
	Instruction_t* newInsn = IRDB_SDK::insertAssemblyBefore(virp, first, assembly, target);
	Function_t* func = newInsn->getFunction();
	inserted_instr[func].insert(newInsn);
        inserted_addr[func].insert(newInsn->getAddress());
        return newInsn;
}


Instruction_t* P1_insertAssemblyBefore(FileIR_t* virp, Instruction_t* first, string assembly)
{
	Instruction_t* newInsn = IRDB_SDK::insertAssemblyBefore(virp, first, assembly);
	Function_t* func = newInsn->getFunction();
	inserted_instr[func].insert(newInsn);
        inserted_addr[func].insert(newInsn->getAddress());
        return newInsn;
}


Instruction_t* P1_insertAssemblyAfter(FileIR_t* virp, Instruction_t* first, string assembly, Instruction_t *target)
{
	Instruction_t* newInsn = IRDB_SDK::insertAssemblyAfter(virp, first, assembly, target);
	Function_t* func = newInsn->getFunction();
	inserted_instr[func].insert(newInsn);
        inserted_addr[func].insert(newInsn->getAddress());
        return newInsn;
}


Instruction_t* P1_insertAssemblyAfter(FileIR_t* virp, Instruction_t* first, string assembly)
{
	Instruction_t* newInsn = IRDB_SDK::insertAssemblyAfter(virp, first, assembly);
	Function_t* func = newInsn->getFunction();
	inserted_instr[func].insert(newInsn);
        inserted_addr[func].insert(newInsn->getAddress());
        return newInsn;
}


Instruction_t* P1_insertDataBitsAfter(FileIR_t* virp, Instruction_t* first, string dataBits, Instruction_t *target)
{
	Instruction_t* newInsn = IRDB_SDK::insertDataBitsAfter(virp, first, dataBits, target);
	Function_t* func = newInsn->getFunction();
	inserted_instr[func].insert(newInsn);
        inserted_addr[func].insert(newInsn->getAddress());
        return newInsn;
}


#if 0
Instruction_t* P1_copyInstruction(Instruction_t* instr)
{
	return IRDB_SDK::copyInstruction(instr);
}


void P1_copyInstruction(Instruction_t* src, Instruction_t* dest)
{
	IRDB_SDK::copyInstruction(src, dest);
}
#endif

Instruction_t* P1_allocateNewInstruction(FileIR_t* virp, DatabaseID_t p_fileID, Function_t* func)
{
//	Instruction_t* newInsn = IRDB_SDK::allocateNewInstruction(virp, p_fileID, func);
	auto newAddr=virp->addNewAddress(virp->getFile()->getBaseID(),0);
	auto newInsn=virp->addNewInstruction(newAddr, func);

	inserted_instr[func].insert(newInsn);
	inserted_addr[func].insert(newInsn->getAddress());
	return newInsn;
}


Instruction_t* P1_allocateNewInstruction(FileIR_t* virp, Instruction_t *template_instr)
{
// 	Instruction_t* newInsn = IRDB_SDK::allocateNewInstruction(virp, template_instr);
	auto fileId=virp->getFile()->getBaseID();
	Function_t* func = template_instr->getFunction();
	auto newInsn=P1_allocateNewInstruction(virp,fileId,func);
	inserted_instr[func].insert(newInsn);
        inserted_addr[func].insert(newInsn->getAddress());
        return newInsn;
}


void P1_setInstructionAssembly(FileIR_t* virp,Instruction_t *p_instr, string p_assembly, Instruction_t *p_fallThrough, Instruction_t *p_target)
{
	IRDB_SDK::setInstructionAssembly(virp, p_instr, p_assembly, p_fallThrough, p_target);
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

Instruction_t* getHandlerCode(FileIR_t* virp, Instruction_t* fallthrough, mitigation_policy policy, unsigned exit_code)
{
	auto handler_code=(Instruction_t *)nullptr;

	if (policy == P_CONTROLLED_EXIT) 
	{
		string exit_code_str = 
			virp->getArchitectureBitWidth()==64 ? 
				"mov rdi, " + std::to_string(exit_code) : 
				"mov ebx, " + std::to_string(exit_code);

		handler_code = P1_allocateNewInstruction(virp,fallthrough);

		P1_setInstructionAssembly(virp,handler_code,exit_code_str.c_str(), NULL,NULL);
		Instruction_t* syscall_num = 
			virp->getArchitectureBitWidth()==64 ? 
				P1_insertAssemblyAfter(virp,handler_code,"mov rax, 60",NULL):  
				P1_insertAssemblyAfter(virp,handler_code,"mov eax, 1",NULL);
		Instruction_t* syscall_i = P1_insertAssemblyAfter(virp,syscall_num,"syscall",NULL);
		syscall_i->setFallthrough(fallthrough);
	}
	else if (policy == P_HARD_EXIT) 
	{
		handler_code= P1_allocateNewInstruction(virp,fallthrough);
		P1_setInstructionAssembly(virp,handler_code,"hlt",NULL,NULL);
		handler_code->setComment("hlt ; hard exit requested");
		handler_code->setFallthrough(fallthrough);
	}
	else
	{
		handler_code= P1_allocateNewInstruction(virp,fallthrough);
		P1_setInstructionAssembly(virp,handler_code,"hlt",NULL,NULL);
		handler_code->setComment("hlt ; Make this into a callback: jdh@getHandlerCode");
		handler_code->setFallthrough(fallthrough);
	}

	return handler_code;
}

Instruction_t* insertCanaryCheckBefore(FileIR_t* virp,Instruction_t *first, unsigned int canary_val, int esp_offset, Instruction_t *fail_code)
{
	auto do_zero=(first->getDisassembly().find("ret")!=string::npos);
	stringstream ss;
	const char *sp_reg="esp";
	if(virp->getArchitectureBitWidth()==64)
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
	Instruction_t* next = P1_insertAssemblyBefore(virp,first,ss.str());

	//Then insert the jmp after the compare. 
	//The fallthrough of the inserted jmp will be a copy of the original
	//instruction, still pointed to by "first".
	P1_insertDataBitsAfter(virp,first,getJnzDataBits(),fail_code);
	first->setComment("Canary Check: "+first->getComment());

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
        if(virp->getArchitectureBitWidth()==64)
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
        Instruction_t* next = P1_insertAssemblyAfter(virp,first,ss.str());
        first->setComment("Canary Zero: "+first->getComment());

        return next;
}

Relocation_t* createNewRelocation(FileIR_t* firp, Instruction_t* insn, string type, int offset)
{
	/*
        Relocation_t* reloc=new Relocation_t;
        insn->getRelocations().insert(reloc);
        firp->getRelocations().insert(reloc);

	reloc->SetType(type);
	reloc->SetOffset(offset);
	*/
	auto reloc=firp->addNewRelocation(insn,offset,type);

        return reloc;
}

