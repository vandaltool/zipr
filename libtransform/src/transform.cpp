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

#include "transform.hpp"
#include "Rewrite_Utility.hpp"
// #include <bea_deprecated.hpp>

/*
 * Find the first occurrence of find in s, ignore case.
 */
static char *
my_strcasestr(const char* s, char *find)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != 0) {
		c = tolower((unsigned char)c);
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == 0)
					return (NULL);
			} while ((char)tolower((unsigned char)sc) != c);
		} while (strncasecmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

#define OPTIMIZE_ASSEMBLY

using namespace libTransform;
using namespace MEDS_Annotation;

// 20130415 Anh added support for additional registers for various utility functions
// 20130415 Anh added assert() statements for unhandled registers

Transform::Transform(VariantID_t *p_variantID, FileIR_t *p_fileIR, set<std::string> *p_filteredFunctions)
{
	m_variantID = p_variantID;                  // Current variant ID
	m_fileIR = p_fileIR;                  		// File IR (off the database) for variant
	m_filteredFunctions = p_filteredFunctions;  // Blacklisted funtions
}

void Transform::addInstruction(Instruction_t *p_instr, string p_dataBits, Instruction_t *p_fallThrough, Instruction_t *p_target)
{
	if (p_instr == NULL) return;

	p_instr->SetDataBits(p_dataBits);
	p_instr->SetComment(p_instr->getDisassembly());
	p_instr->SetFallthrough(p_fallThrough); 
	p_instr->SetTarget(p_target); 

	m_fileIR->GetAddresses().insert(p_instr->GetAddress());
	m_fileIR->GetInstructions().insert(p_instr);
}

#define BENSWAY

//
//  Before:                              After:
//  <p_instrumented> ["mov edx,1"]       <p_newInstr> []
//                                       <dupInstr>   ["mov edx,1"] <-- returns
//
Instruction_t* Transform::carefullyInsertBefore(Instruction_t* &p_instrumented, Instruction_t * &p_newInstr)
{

//For all insertBefore functions:
//The "first" instruction will have its contents replaced and a duplicate of "first" will be in the follow of first. 
//This duplicate is returned since the user already has a pointer to first.
//To insert before an instruction is the same as modifying the original instruction, and inserting after it
//a copy of the original instruction 

#ifdef BENSWAY
	Instruction_t* i2 = IRDBUtility::insertAssemblyBefore(m_fileIR, p_instrumented, m_fileIR->LookupAssembly(p_newInstr), p_newInstr->GetTarget());
cerr << "carefullyInsertBefore (Ben's Way): @: 0x" << std::hex << p_instrumented->GetAddress() << std::dec << " old instruction: " << p_instrumented->getDisassembly() << " new instruction: " << m_fileIR->LookupAssembly(p_newInstr) << endl;
	return i2;
#else
	db_id_t fileID = p_instrumented->GetAddress()->GetFileID();
	Function_t* func = p_instrumented->GetFunction();

	assert(p_instrumented && p_newInstr && p_instrumented->GetAddress());

// why is old instrunction blank?
cerr << "(1) carefullyInsertBefore: @: 0x" << std::hex << p_instrumented->GetAddress() << std::dec << " old instruction: " << p_instrumented->getDisassembly() << " new instruction: " << m_fileIR->LookupAssembly(p_newInstr) << endl;

	// duplicate old instrumented instruction
	Instruction_t* dupInstr = allocateNewInstruction(fileID, func);

	dupInstr->SetDataBits(p_instrumented->GetDataBits());
	dupInstr->SetComment(p_instrumented->GetComment());
	dupInstr->SetCallback(p_instrumented->GetCallback());
	dupInstr->SetFallthrough(p_instrumented->GetFallthrough());
	dupInstr->SetTarget(p_instrumented->GetTarget());
	dupInstr->SetOriginalAddressID(p_instrumented->GetOriginalAddressID());
	AddressID_t *saveIBTA = p_instrumented->GetIndirectBranchTargetAddress();
	dupInstr->SetIndirectBranchTargetAddress(NULL);

	//
	//  pre: p_instrument --> "mov edx, 1"
	//	
	// post: p_instrument no longer in map
	//       new --> "mov edx, 1"
	//
	// this function is equivalent to:
	//      m_fileIR->UnregisterAssembly(p_instrumented);
	//      m_fileIR->RegisterAssembly(dupInstr, newAssemblyCode);
	//
	m_fileIR->ChangeRegistryKey(p_instrumented, dupInstr);

	if (p_newInstr->GetDataBits().size() == 0)
		m_fileIR->RegisterAssembly(p_instrumented, m_fileIR->LookupAssembly(p_newInstr));
	else
		p_instrumented->SetDataBits(p_newInstr->GetDataBits()); 

	p_instrumented->SetComment(p_newInstr->GetComment());
	p_instrumented->SetCallback(p_newInstr->GetCallback());
	p_instrumented->SetFallthrough(dupInstr);

	p_instrumented->SetOriginalAddressID(BaseObj_t::NOT_IN_DATABASE);
	p_instrumented->SetIndirectBranchTargetAddress(saveIBTA);
	p_instrumented->GetRelocations().clear();

   	p_newInstr = p_instrumented;

cerr << "(2) carefullyInsertBefore: @: 0x" << std::hex << p_instrumented->GetAddress() << std::dec << " old instruction: " << dupInstr->getDisassembly() << " new instruction: " << m_fileIR->LookupAssembly(p_newInstr) << endl;
	return dupInstr;
#endif
}

void Transform::addPushf(Instruction_t *p_pushf_i, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(1);
	dataBits[0] = 0x9c;
	addInstruction(p_pushf_i, dataBits, p_fallThrough, NULL);
}

void Transform::addPushRegister(Instruction_t *p_instr, RegisterName p_reg, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(1);

	if (p_reg == rn_EAX || p_reg == rn_RAX)
	{
		dataBits[0] = 0x50; 
	} 
	else if (p_reg == rn_EBX || p_reg == rn_RBX)
	{
		dataBits[0] = 0x53; 
	} 
	else if (p_reg == rn_ECX || p_reg == rn_RCX)
	{
		dataBits[0] = 0x51; 
	}
	else if (p_reg == rn_EDX || p_reg == rn_RDX)
	{
		dataBits[0] = 0x52; 
	}
	else if (p_reg == rn_ESI || p_reg == rn_RSI)
	{
		dataBits[0] = 0x56; 
	}
	else if (p_reg == rn_EDI || p_reg == rn_RDI)
	{
		dataBits[0] = 0x57; 
	}
	else if (p_reg == rn_EBP || p_reg == rn_RBP)
	{
		dataBits[0] = 0x55; 
	}
	else if (p_reg == rn_ESP || p_reg == rn_RSP)
	{
		dataBits[0] = 0x54; 
	}
	else
	{
		dataBits.resize(2);
		dataBits[0] = 0x41;
		if (p_reg == rn_R8)
		{
			dataBits[1] = 0x50;
		} 
		else if (p_reg == rn_R9)
		{
			dataBits[1] = 0x51;
		} 
		else if (p_reg == rn_R10)
		{
			dataBits[1] = 0x52;
		} 
		else if (p_reg == rn_R11)
		{
			dataBits[1] = 0x53;
		} 
		else if (p_reg == rn_R12)
		{
			dataBits[1] = 0x54;
		} 
		else if (p_reg == rn_R13)
		{
			dataBits[1] = 0x55;
		} 
		else if (p_reg == rn_R14)
		{
			dataBits[1] = 0x56;
		} 
		else if (p_reg == rn_R15)
		{
			dataBits[1] = 0x57;
		}
		else
		{
			cerr << "Transform::addPushRegister: unhandled register: " << p_reg << endl;
			assert(0);
			return;
		}
	}

	addInstruction(p_instr, dataBits, p_fallThrough, NULL);
}

void Transform::addPopRegister(Instruction_t *p_instr, RegisterName p_reg, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(1);

	if (p_reg == rn_EAX || p_reg == rn_RAX)
	{
		dataBits[0] = 0x58; 
	} 
	else if (p_reg == rn_EBX || p_reg == rn_RBX)
	{
		dataBits[0] = 0x5b; 
	} 
	else if (p_reg == rn_ECX || p_reg == rn_RCX)
	{
		dataBits[0] = 0x59; 
	}
	else if (p_reg == rn_EDX || p_reg == rn_RDX)
	{
		dataBits[0] = 0x5a; 
	}
	else if (p_reg == rn_ESI || p_reg == rn_RSI)
	{
		dataBits[0] = 0x5e; 
	}
	else if (p_reg == rn_EDI || p_reg == rn_RDI)
	{
		dataBits[0] = 0x5f; 
	}
	else if (p_reg == rn_EBP || p_reg == rn_RBP)
	{
		dataBits[0] = 0x5d; 
	}
	else if (p_reg == rn_ESP || p_reg == rn_RSP)
	{
		dataBits[0] = 0x5c; 
	}
	else
	{
		dataBits.resize(2);
		dataBits[0] = 0x41;
		if (p_reg == rn_R8)
		{
			dataBits[1] = 0x58;
		} 
		else if (p_reg == rn_R9)
		{
			dataBits[1] = 0x59;
		} 
		else if (p_reg == rn_R10)
		{
			dataBits[1] = 0x5a;
		} 
		else if (p_reg == rn_R11)
		{
			dataBits[1] = 0x5b;
		} 
		else if (p_reg == rn_R12)
		{
			dataBits[1] = 0x5c;
		} 
		else if (p_reg == rn_R13)
		{
			dataBits[1] = 0x5d;
		} 
		else if (p_reg == rn_R14)
		{
			dataBits[1] = 0x5e;
		} 
		else if (p_reg == rn_R15)
		{
			dataBits[1] = 0x5f;
		}
		else
		{
			cerr << "Transform::addPopRegister: unhandled register";
			assert(0);
			return;
		}
	}

	addInstruction(p_instr, dataBits, p_fallThrough, NULL);
}

void Transform::addPusha(Instruction_t *p_pusha_i, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(1);
	dataBits[0] = 0x60;
	addInstruction(p_pusha_i, dataBits, p_fallThrough, NULL);
}

void Transform::addPopf(Instruction_t *p_popf_i, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(1);
	dataBits[0] = 0x9d;
	addInstruction(p_popf_i, dataBits, p_fallThrough, NULL);
}

void Transform::addPopa(Instruction_t *p_popa_i, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(1);
	dataBits[0] = 0x61;
	addInstruction(p_popa_i, dataBits, p_fallThrough, NULL);
}

void Transform::addNop(Instruction_t *p_nop_i, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(1);
	dataBits[0] = 0x90;
	p_nop_i->SetComment(string("NOP"));
	addInstruction(p_nop_i, dataBits, p_fallThrough, NULL);
}

Instruction_t* Transform::allocateNewInstruction(db_id_t p_fileID, Function_t* p_func)
{
	Instruction_t *instr = new Instruction_t();
	AddressID_t *a = new AddressID_t();

	a->SetFileID(p_fileID);

	instr->SetFunction(p_func);
	instr->SetAddress(a);

	m_fileIR->GetInstructions().insert(instr);
	m_fileIR->GetAddresses().insert(a);
	return instr;
}

virtual_offset_t Transform::getAvailableAddress()
{
/*
	// traverse all instructions
	// grab address

	// @todo: lookup instruction size so that we don't waste any space
	// for some reason the max available address is incorrect! was ist los?

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
// availableAddressOffset + 16;
*/

	static int counter = -16;
	counter += 16;
	return 0xf0000000 + counter;
}

void Transform::addCallbackHandler(string p_detector, Instruction_t *p_instrumentedInstruction, Instruction_t *p_instruction, Instruction_t *p_fallThrough, int p_policy, AddressID_t *p_addressOriginalInstruction)
{
	assert(getFileIR() && p_instruction && p_fallThrough);

	string dataBits;

	db_id_t fileID = p_instruction->GetAddress()->GetFileID();
	Function_t* func = p_instruction->GetFunction();

 	// create and register new instructions (and addresses)
	Instruction_t* pushf_i = allocateNewInstruction(fileID, func);
	Instruction_t* pusha_i = allocateNewInstruction(fileID, func);
	Instruction_t* pushPolicy_i = allocateNewInstruction(fileID, func);
	Instruction_t* pusharg_i = allocateNewInstruction(fileID, func);
	Instruction_t* pushret_i = allocateNewInstruction(fileID, func);
	Instruction_t* poparg_i = allocateNewInstruction(fileID, func);
	Instruction_t* popPolicy_i = allocateNewInstruction(fileID, func);
	Instruction_t* popa_i = allocateNewInstruction(fileID, func);
	Instruction_t* popf_i = allocateNewInstruction(fileID, func);

	// pin the poparg instruction 
	virtual_offset_t postDetectorReturn = getAvailableAddress();
	poparg_i->GetAddress()->SetVirtualOffset(postDetectorReturn);

	// link callback handler sequence to instrumented instruction
	p_instruction->SetFallthrough(pushf_i);
	p_instruction->SetComment(p_instruction->GetComment() + " -- start of callback handler sequence");

	// pushf   
	addPushf(pushf_i, pusha_i);

	// pusha   
	addPusha(pusha_i, pushPolicy_i);

	// push detector exit policy
	//     0 - default
	//     1 - continue
	//     2 - exit
	//     3 - saturating arithmetic
	dataBits.resize(5);
	dataBits[0] = 0x68;
	int *tmpi = (int *) &dataBits[1];
	*tmpi = p_policy;
	pushPolicy_i->SetDataBits(dataBits);
	pushPolicy_i->SetComment(pushPolicy_i->getDisassembly() + string(" - policy spec"));
	pushPolicy_i->SetFallthrough(pusharg_i); 

	// push (PC of instrumented instruction)
	dataBits.resize(5);
	dataBits[0] = 0x68;
	virtual_offset_t *tmp = (virtual_offset_t *) &dataBits[1];
	if (p_addressOriginalInstruction)
		*tmp = p_addressOriginalInstruction->GetVirtualOffset();
	else
		*tmp = p_instrumentedInstruction->GetAddress()->GetVirtualOffset();
	pusharg_i->SetDataBits(dataBits);
	pusharg_i->SetComment(pusharg_i->getDisassembly());
	pusharg_i->SetFallthrough(pushret_i); 

	// pushret   
	dataBits.resize(5);
	dataBits[0] = 0x68;
	tmp = (virtual_offset_t *) &dataBits[1];
	*tmp = postDetectorReturn;
	pushret_i->SetDataBits(dataBits);
	pushret_i->SetComment(pushret_i->getDisassembly());
	pushret_i->SetFallthrough(poparg_i); 

	// poparg
	dataBits.resize(1);
	dataBits[0] = 0x58;
	poparg_i->SetDataBits(dataBits);
	poparg_i->SetComment(poparg_i->getDisassembly() + " -- with callback to " + p_detector + " orig: " + p_instruction->GetComment()) ;
	poparg_i->SetFallthrough(popPolicy_i); 
	poparg_i->SetCallback(p_detector); 
	AddressID_t *poparg_i_indTarg =new AddressID_t();
	m_fileIR->GetAddresses().insert(poparg_i_indTarg);
	poparg_i_indTarg->SetVirtualOffset(poparg_i->GetAddress()->GetVirtualOffset());
	poparg_i_indTarg->SetFileID(BaseObj_t::NOT_IN_DATABASE);
	poparg_i->SetIndirectBranchTargetAddress(poparg_i_indTarg);

	// popPolicy
	dataBits.resize(1);
	dataBits[0] = 0x58;
	popPolicy_i->SetDataBits(dataBits);
	popPolicy_i->SetFallthrough(popa_i); 

	// popa   
	addPopa(popa_i, popf_i);

	// popf   
	addPopf(popf_i, p_fallThrough);
}


#if 0
// returns true if BeaEngine says arg1 of the instruction is a register 
bool Transform::hasTargetRegister(Instruction_t *p_instruction, int p_argNo)
{
	if (!p_instruction)
		return false;

	DISASM disasm;
	Disassemble(p_instruction,disasm);
	
	if (p_argNo == 1)
		return disasm.Argument1.ArgType & 0xFFFF0000 & REGISTER_TYPE;
	else if (p_argNo == 2)
		return disasm.Argument2.ArgType & 0xFFFF0000 & REGISTER_TYPE;
	else if (p_argNo == 3)
		return disasm.Argument3.ArgType & 0xFFFF0000 & REGISTER_TYPE;
	else
		return false;
}


RegisterName Transform::getTargetRegister(Instruction_t *p_instruction, int p_argNo)
{
	if (hasTargetRegister(p_instruction, p_argNo))
	{
		DISASM disasm;
		Disassemble(p_instruction,disasm);

		if (p_argNo == 1)
			return Register::getRegister(disasm.Argument1.ArgMnemonic);
		else if (p_argNo == 2)
			return Register::getRegister(disasm.Argument2.ArgMnemonic);
		else if (p_argNo == 3)
			return Register::getRegister(disasm.Argument3.ArgMnemonic);
	}
	else
		return rn_UNKNOWN;
}

//
// Returns true iff instruction is MUL (according to BeaEngine)
//
bool Transform::isMultiplyInstruction(Instruction_t *p_instruction)
{
	if (!p_instruction)
		return false;

	std::string assembly = m_fileIR->LookupAssembly(p_instruction);
	if (assembly.length() > 0)
	{
		return my_strcasestr(assembly.c_str(), "MUL") != NULL;
	}

	DISASM disasm;
	Disassemble(p_instruction,disasm);

	// beaengine adds space at the end of the mnemonic string
	return my_strcasestr(disasm.Instruction.Mnemonic, "MUL ") != NULL;
}


//
// Returns true iff instruction is a MOV (according to BeaEngine)
//
bool Transform::isMovInstruction(Instruction_t *p_instruction)
{
	if (!p_instruction)
		return false;

	std::string assembly = m_fileIR->LookupAssembly(p_instruction);
	if (assembly.length() > 0)
	{
		return my_strcasestr(assembly.c_str(), "MOV") != NULL;
	}

	DISASM disasm;
	Disassemble(p_instruction,disasm);

	// nb: beaengine adds space at the end of the mnemonic string
	return my_strcasestr(disasm.Instruction.Mnemonic, "MOV") != NULL;
}

//
// Returns true iff instruction is ADD or SUB (according to BeaEngine)
//
bool Transform::isAddSubNonEspInstruction(Instruction_t *p_instruction)
{
	if (!p_instruction)
		return false;

	DISASM disasm;
	Disassemble(p_instruction,disasm);

	// beaengine adds space at the end of the mnemonic string
	if (my_strcasestr(disasm.Instruction.Mnemonic, "ADD "))
	{
		return true;
	}
	else if (my_strcasestr(disasm.Instruction.Mnemonic, "SUB ")) 
	{
		if (my_strcasestr(disasm.Argument1.ArgMnemonic,"esp") &&
			(disasm.Argument2.ArgType & 0xFFFF0000 & (CONSTANT_TYPE | ABSOLUTE_)))
		{
			// optimization: filter out "sub esp, K"
			return false;
		}
		return true;
	}

	return false;
}
#endif

void Transform::addTestRegister8(Instruction_t *p_instr, RegisterName p_reg, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(2);
	if (p_reg == rn_AL)
	{
		dataBits[0] = 0x84;
		dataBits[1] = 0xc0;
	}
	else if (p_reg == rn_BL)
	{
		dataBits[0] = 0x84;
		dataBits[1] = 0xdb;
	}
	else if (p_reg == rn_CL)
	{
		dataBits[0] = 0x84;
		dataBits[1] = 0xc9;
	}
	else if (p_reg == rn_DL)
	{
		dataBits[0] = 0x84;
		dataBits[1] = 0xd2;
	}
	else if (p_reg == rn_AH)
	{
		dataBits[0] = 0x84;
		dataBits[1] = 0xe4;
	}
	else if (p_reg == rn_BH)
	{
		dataBits[0] = 0x84;
		dataBits[1] = 0xff;
	}
	else if (p_reg == rn_CH)
	{
		dataBits[0] = 0x84;
		dataBits[1] = 0xed;
	}
	else if (p_reg == rn_DH)
	{
		dataBits[0] = 0x84;
		dataBits[1] = 0xf6;
	}
	else
	{
		cerr << "Transform::addTestRegister8(): unhandled register" << endl;
		assert(0);
		return;
	}

	addInstruction(p_instr, dataBits, p_fallThrough, NULL);
}

void Transform::addTestRegister16(Instruction_t *p_instr, RegisterName p_reg, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(3);
	if (p_reg == rn_AX)
	{
		dataBits[0] = 0x66;
		dataBits[1] = 0x85;
		dataBits[2] = 0xc0;
	}
	else if (p_reg == rn_BX)
	{
		dataBits[0] = 0x66;
		dataBits[1] = 0x85;
		dataBits[2] = 0xdb;
	}
	else if (p_reg == rn_CX)
	{
		dataBits[0] = 0x66;
		dataBits[1] = 0x85;
		dataBits[2] = 0xc9;
	}
	else if (p_reg == rn_DX)
	{
		dataBits[0] = 0x66;
		dataBits[1] = 0x85;
		dataBits[2] = 0xd2;
	}
	else if (p_reg == rn_BP)
	{
		assert(0);
	}
	else if (p_reg == rn_SP)
	{
		assert(0);
	}
	else if (p_reg == rn_SI)
	{
		assert(0);
	}
	else if (p_reg == rn_DI)
	{
		assert(0);
	}
	else
	{
		cerr << "Transform::addTestRegister16(): unhandled register" << endl;
		assert(0);
		return;
	}

	addInstruction(p_instr, dataBits, p_fallThrough, NULL);
}

// test <reg32>, <reg32>
void Transform::addTestRegister32(Instruction_t *p_instr, RegisterName p_reg, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(2);
	if (p_reg == rn_EAX)
	{
		dataBits[0] = 0x85;
		dataBits[1] = 0xc0;
	}
	else if (p_reg == rn_EBX)
	{
		dataBits[0] = 0x85;
		dataBits[1] = 0xdb;
	}
	else if (p_reg == rn_ECX)
	{
		dataBits[0] = 0x85;
		dataBits[1] = 0xc9;
	}
	else if (p_reg == rn_EDX)
	{
		dataBits[0] = 0x85;
		dataBits[1] = 0xd2;
	}
	else if (p_reg == rn_ESI)
	{
		dataBits[0] = 0x85;
		dataBits[1] = 0xf6;
	}
	else if (p_reg == rn_EDI)
	{
		dataBits[0] = 0x85;
		dataBits[1] = 0xff;
	}
	else if (p_reg == rn_EBP)
	{
		dataBits[0] = 0x85;
		dataBits[1] = 0xed;
	}
	else if (p_reg == rn_ESP)
	{
		dataBits[0] = 0x85;
		dataBits[1] = 0xe4;
	}
	else
	{
		cerr << "Transform::addTestRegister32(): unhandled register" << endl;
		assert(0);
		return;
	}

	addInstruction(p_instr, dataBits, p_fallThrough, NULL);
}


void Transform::addTestRegister(Instruction_t *p_instr, RegisterName p_reg, Instruction_t *p_fallThrough)
{
	if (Register::is8bit(p_reg))
		addTestRegister8(p_instr, p_reg, p_fallThrough);
	else if (Register::is16bit(p_reg))
		addTestRegister16(p_instr, p_reg, p_fallThrough);
	else if (Register::is32bit(p_reg))
		addTestRegister32(p_instr, p_reg, p_fallThrough);
}

void Transform::addTestRegisterMask(Instruction_t *p_instr, RegisterName p_reg, unsigned p_mask, Instruction_t *p_fallThrough)
{
	if (Register::is32bit(p_reg))
		addTestRegisterMask32(p_instr, p_reg, p_mask, p_fallThrough);
}

void Transform::addTestRegisterMask32(Instruction_t *p_instr, RegisterName p_reg, unsigned p_mask, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(6); // all but EAX take 6 bytes
	unsigned *tmp;

	if (p_reg == rn_EAX)
	{
		dataBits.resize(5);
		dataBits[0] = 0xa9;
		tmp = (unsigned *) &dataBits[1];
		*tmp = p_mask;
	}
	else if (p_reg == rn_EBX)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xc3;
		tmp = (unsigned *) &dataBits[2];
		*tmp = p_mask;
	}
	else if (p_reg == rn_ECX)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xc1;
		tmp = (unsigned *) &dataBits[2];
		*tmp = p_mask;
	}
	else if (p_reg == rn_EDX)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xc2;
		tmp = (unsigned *) &dataBits[2];
		*tmp = p_mask;
	}
	else if (p_reg == rn_ESI)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xc6;
		tmp = (unsigned *) &dataBits[2];
		*tmp = p_mask;
	}
	else if (p_reg == rn_EDI)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xc7;
		tmp = (unsigned *) &dataBits[2];
		*tmp = p_mask;
	}
	else if (p_reg == rn_EBP)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xc5;
		tmp = (unsigned *) &dataBits[2];
		*tmp = p_mask;
	}
	else if (p_reg == rn_ESP)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xc4;
		tmp = (unsigned *) &dataBits[2];
		*tmp = p_mask;
	}
	else
	{
		cerr << "Transform::addTestRegisterMask32(): unhandled register: " << p_reg << endl;
		assert(0);
		return;
	}

	addInstruction(p_instr, dataBits, p_fallThrough, NULL);
}

void Transform::addCmpRegisterMask(Instruction_t *p_instr, RegisterName p_reg, unsigned p_mask, Instruction_t *p_fallThrough)
{
	if (Register::is32bit(p_reg))
		addCmpRegisterMask32(p_instr, p_reg, p_mask, p_fallThrough);
}

void Transform::addCmpRegisterMask32(Instruction_t *p_instr, RegisterName p_reg, unsigned p_mask, Instruction_t *p_fallThrough)
{
	string dataBits;
	unsigned *tmp;

	if (p_reg == rn_EAX)
	{
      dataBits.resize(5); // EAX gets compact encoding
		dataBits[0] = 0x3d;
		tmp = (unsigned *) &dataBits[1];
		*tmp = p_mask;
	}
	else 
   { // common code for non-EAX cases first
	  dataBits.resize(6); // all but EAX take 6 bytes
     dataBits[0] = 0x81;
     tmp = (unsigned *) &dataBits[2];
     *tmp = p_mask;
     if (p_reg == rn_EBX)
	  {
       dataBits[1] = 0xfb;
     }
     else if (p_reg == rn_ECX)
     {
       dataBits[1] = 0xf9;
     }
     else if (p_reg == rn_EDX)
     {
       dataBits[1] = 0xfa;
     }
     else if (p_reg == rn_EBP)
     {
       dataBits[1] = 0xfd;
     }
     else if (p_reg == rn_ESI)
     {
       dataBits[1] = 0xfe;
     }
     else if (p_reg == rn_EDI)
     {
       dataBits[1] = 0xff;
     }
     else
     {
       cerr << "Transform::addCmpRegisterMask32(): unhandled register" << endl;
		assert(0);
       return;
     }
   }

	addInstruction(p_instr, dataBits, p_fallThrough, NULL);
}

// jns - jump not signed
void Transform::addJns(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target)
{
	string dataBits;
	dataBits.resize(2);
	dataBits[0] = 0x79;
	dataBits[1] = 0x00; // value doesn't matter -- we will fill it in later
	addInstruction(p_instr, dataBits, p_fallThrough, p_target);
}

// jz - jump zero (same as je - jump if equal)
void Transform::addJz(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target)
{
	string dataBits;
	dataBits.resize(2);
	dataBits[0] = 0x74;
	dataBits[1] = 0x00; // value doesn't matter -- we will fill it in later

	addInstruction(p_instr, dataBits, p_fallThrough, p_target);
}

// jnz - jump not zero (same as jne - jump if not equal)
void Transform::addJnz(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target)
{
	string dataBits;
	dataBits.resize(2);
	dataBits[0] = 0x75;
	dataBits[1] = 0x00; // value doesn't matter -- we will fill it in later

	addInstruction(p_instr, dataBits, p_fallThrough, p_target);
}

// jae - jump if above or equal (unsigned) (same as jnb and jnc)
void Transform::addJae(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target)
{
	string dataBits;
	dataBits.resize(2);
	dataBits[0] = 0x73;
	dataBits[1] = 0x00; // value doesn't matter -- we will fill it in later

	addInstruction(p_instr, dataBits, p_fallThrough, p_target);
}

// jo - jump overflow
void Transform::addJo(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target)
{
	string dataBits;
	dataBits.resize(2);
	dataBits[0] = 0x70;
	dataBits[1] = 0x00; // value doesn't matter -- we will fill it in later

	addInstruction(p_instr, dataBits, p_fallThrough, p_target);
}
// jc - jump carry
void Transform::addJc(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target)
{
	string dataBits;
	dataBits.resize(2);
	dataBits[0] = 0x72;
	dataBits[1] = 0x00; // value doesn't matter -- we will fill it in later

	addInstruction(p_instr, dataBits, p_fallThrough, p_target);
}

// not <reg> -- negate register
void Transform::addNot(Instruction_t *p_instr, RegisterName p_reg, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(2); 

	if (p_reg == rn_EAX)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xd0;
	}
	else if (p_reg == rn_EBX)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xd3;
	}
	else if (p_reg == rn_ECX)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xd1;
	}
	else if (p_reg == rn_EDX)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xd2;
	}
	else if (p_reg == rn_ESI)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xd6;
	}
	else if (p_reg == rn_EDI)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xd7;
	}
	else if (p_reg == rn_AX)
	{
		dataBits.resize(3); 
		dataBits[0] = 0x66;
		dataBits[1] = 0xf7;
		dataBits[2] = 0xd0;
	}
	else if (p_reg == rn_BX)
	{
		dataBits.resize(3); 
		dataBits[0] = 0x66;
		dataBits[1] = 0xf7;
		dataBits[2] = 0xd3;
	}
	else if (p_reg == rn_CX)
	{
		dataBits.resize(3); 
		dataBits[0] = 0x66;
		dataBits[1] = 0xf7;
		dataBits[2] = 0xd1;
	}
	else if (p_reg == rn_DX)
	{
		dataBits.resize(3); 
		dataBits[0] = 0x66;
		dataBits[1] = 0xf7;
		dataBits[2] = 0xd2;
	}
	else if (p_reg == rn_AL)
	{
		dataBits[0] = 0xf6;
		dataBits[1] = 0xd0;
	}
	else if (p_reg == rn_BL)
	{
		dataBits[0] = 0xf6;
		dataBits[1] = 0xd3;
	}
	else if (p_reg == rn_CL)
	{
		dataBits[0] = 0xf6;
		dataBits[1] = 0xd1;
	}
	else if (p_reg == rn_DL)
	{
		dataBits[0] = 0xf6;
		dataBits[1] = 0xd2;
	}
	else if (p_reg == rn_AH)
	{
		dataBits[0] = 0xf6;
		dataBits[1] = 0xd4;
	}
	else if (p_reg == rn_BH)
	{
		dataBits[0] = 0xf6;
		dataBits[1] = 0xd7;
	}
	else if (p_reg == rn_CH)
	{
		dataBits[0] = 0xf6;
		dataBits[1] = 0xd5;
	}
	else if (p_reg == rn_DH)
	{
		dataBits[0] = 0xf6;
		dataBits[1] = 0xd6;
	}

	else
	{
		cerr << "Transform::addNot(): unhandled register" << endl;
		assert(0);
		return;
	}

	addInstruction(p_instr, dataBits, p_fallThrough, NULL);
}

// add r1, r2
void Transform::addAddRegisters(Instruction_t *p_instr, RegisterName p_regTgt, RegisterName p_regSrc, Instruction_t *p_fallThrough)
{
	// too many combinations, just use the assembler
	string assembly = "add " + Register::toString(p_regTgt) + ", " + Register::toString(p_regSrc);
#ifdef OPTIMIZE_ASSEMBLY
	m_fileIR->RegisterAssembly(p_instr, assembly);
#else
	if (!p_instr->Assemble(assembly))
	{
		cerr << "addAddRegisters(): error in assembling instruction: " << assembly << endl;
		assert(0);
		return;
	}
#endif

	p_instr->SetFallthrough(p_fallThrough);
}

// add r1, constant
void Transform::addAddRegisterConstant(Instruction_t *p_instr, RegisterName p_reg, int p_constantValue, Instruction_t *p_fallThrough)
{
	// too many combinations, just use the assembler
	char buf[256];
	sprintf(buf, "add %s, %d", Register::toString(p_reg).c_str(), p_constantValue);
	string assembly(buf);
#ifdef OPTIMIZE_ASSEMBLY
	m_fileIR->RegisterAssembly(p_instr, assembly);
#else
	if (!p_instr->Assemble(assembly))
	{
		cerr << "Transform::addAddConstant(): error in assembling instruction: " << assembly << endl;
		assert(0);
		return;
	}
#endif

//	cerr << "Transform::addAddConstant(): " << p_instr->getDisassembly() << endl;
	p_instr->SetFallthrough(p_fallThrough);
}

// imul r1, constant
void Transform::addMulRegisterConstant(Instruction_t *p_instr, RegisterName p_reg, int p_constantValue, Instruction_t *p_fallThrough)
{
	// too many combinations, just use the assembler
	char buf[256];
	sprintf(buf, "imul %s, %d", Register::toString(p_reg).c_str(), p_constantValue);
	string assembly(buf);
#ifdef OPTIMIZE_ASSEMBLY
	m_fileIR->RegisterAssembly(p_instr, assembly);
#else
	if (!p_instr->Assemble(assembly))
	{
		cerr << "Transform::addMulRegisterConstant(): error in assembling instruction: " << assembly << endl;
		assert(0);
		return;
	}
#endif

//	cerr << "Transform::addMulRegisterConstant(): " << p_instr->getDisassembly() << endl;
	p_instr->SetFallthrough(p_fallThrough);
}


// mov r1, r2
void Transform::addMovRegisters(Instruction_t *p_instr, RegisterName p_regTgt, RegisterName p_regSrc, Instruction_t *p_fallThrough)
{
	// too many combinations, just use the assembler
	string assembly = "mov " + Register::toString(p_regTgt) + ", " + Register::toString(p_regSrc);
#ifdef OPTIMIZE_ASSEMBLY
	m_fileIR->RegisterAssembly(p_instr, assembly);
#else
	if (!p_instr->Assemble(assembly))
	{
		cerr << "addMovRegisters(): error in assembling instruction: " << assembly << endl;
		assert(0);
		return;
	}
#endif
	p_instr->SetFallthrough(p_fallThrough);
//	cerr << "addMovRegisters(): " << p_instr->getDisassembly() << endl;
}

void Transform::addMovRegisterSignedConstant(Instruction_t *p_instr, RegisterName p_regTgt, long int p_constant, Instruction_t *p_fallThrough)
{
	p_instr->SetFallthrough(p_fallThrough);

	char buf[128];

	sprintf(buf,"mov %s, %ld", Register::toString(p_regTgt).c_str(), p_constant);

	string assembly(buf);
	m_fileIR->RegisterAssembly(p_instr, assembly);

	p_instr->SetComment("Saturating arithmetic");
}

void Transform::addMovRegisterUnsignedConstant(Instruction_t *p_instr, RegisterName p_regTgt, unsigned long int p_constant, Instruction_t *p_fallThrough)
{
	p_instr->SetFallthrough(p_fallThrough);

	char buf[128];
	sprintf(buf,"mov %s, %lu", Register::toString(p_regTgt).c_str(), p_constant);

	string assembly(buf);
	m_fileIR->RegisterAssembly(p_instr, assembly);

	p_instr->SetComment("Saturating arithmetic");
}

void Transform::addAndRegister32Mask(Instruction_t *p_instr, RegisterName p_regTgt, unsigned int p_mask, Instruction_t *p_fallThrough)
{
    p_instr->SetFallthrough(p_fallThrough);

	char buf[128];
	sprintf(buf,"and %s, 0x%08X", Register::toString(p_regTgt).c_str(), p_mask);

    string assembly(buf);
    cerr << "addAndRegisterMask(): assembling instruction: " << assembly << endl;
#ifdef OPTIMIZE_ASSEMBLY
	m_fileIR->RegisterAssembly(p_instr, assembly);
#else
    if (!p_instr->Assemble(assembly))
    {
        cerr << "addAndRegisterMask(): error in assembling instruction: " << assembly << endl;
		assert(0);
        return;
    }
#endif

	p_instr->SetComment("Saturating arithmetic by masking");
}

//-----------------------------------------------------
// known to be used on x86-64
//-----------------------------------------------------

// hlt
void Transform::addHlt(Instruction_t *p_instr, Instruction_t *p_fallThrough)
{
	string assembly("hlt");
	m_fileIR->RegisterAssembly(p_instr, assembly);
	p_instr->SetFallthrough(p_fallThrough);
}

// jno - jump not overflow
void Transform::addJno(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target)
{
	string assembly("jno 0x22");
	m_fileIR->RegisterAssembly(p_instr, assembly);
	p_instr->SetFallthrough(p_fallThrough);
	p_instr->SetTarget(p_target);
}

// jnc - jump not carry
void Transform::addJnc(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target)
{
	string assembly("jnc 0x22");
	m_fileIR->RegisterAssembly(p_instr, assembly);
	p_instr->SetFallthrough(p_fallThrough);
	p_instr->SetTarget(p_target);
}

Instruction_t* Transform::addNewMaxSaturation(Instruction_t *p_prev, RegisterName p_reg, const MEDS_InstructionCheckAnnotation p_annotation)
{
	Instruction_t *mov = allocateNewInstruction(p_prev->GetAddress()->GetFileID(), p_prev->GetFunction());
	if (p_prev)
		p_prev->SetFallthrough(mov);
	addMaxSaturation(mov, p_reg, p_annotation, NULL);
	return mov;
}

void Transform::addMaxSaturation(Instruction_t *p_instruction, RegisterName p_reg, const MEDS_InstructionCheckAnnotation& p_annotation, Instruction_t *p_fallthrough)
{
	assert(getFileIR() && p_instruction);

	p_instruction->SetFallthrough(p_fallthrough);

	if (p_annotation.isUnsigned())
	{
		// use MAX_UNSIGNED for the bit width
		switch (Register::getBitWidth(p_reg))
		{
			case 64:
				addMovRegisterUnsignedConstant(p_instruction, p_reg, 0xFFFFFFFFFFFFFFFF, p_fallthrough);
				break;
			case 32:
				addMovRegisterUnsignedConstant(p_instruction, p_reg, 0xFFFFFFFF, p_fallthrough);
				break;
			case 16:
				addMovRegisterUnsignedConstant(p_instruction, p_reg, 0xFFFF, p_fallthrough);
				break;
			case 8:
				addMovRegisterUnsignedConstant(p_instruction, p_reg, 0xFF, p_fallthrough);
				break;
			default:
				cerr << "Transform::addMaxSaturation(): invalid bit width: " << p_annotation.getBitWidth() << endl;
				break;
		}
	}
	else
	{
		// treat unknown and signed the same way for overflows
		// use MAX_SIGNED for the bit width
		switch (Register::getBitWidth(p_reg))
		{
			case 64:
				addMovRegisterSignedConstant(p_instruction, p_reg, 0x7FFFFFFFFFFFFFFF, p_fallthrough);
				break;
			case 32:
				addMovRegisterSignedConstant(p_instruction, p_reg, 0x7FFFFFFF, p_fallthrough);
				break;
			case 16:
				addMovRegisterSignedConstant(p_instruction, p_reg, 0x7FFF, p_fallthrough);
				break;
			case 8:
				addMovRegisterSignedConstant(p_instruction, p_reg, 0x7F, p_fallthrough);
				break;
			default:
				cerr << "Transform::addMaxSaturation(): invalid bit width: " << p_annotation.getBitWidth() << endl;
				break;
		}
	}
}

void Transform::addMinSaturation(Instruction_t *p_instruction, RegisterName p_reg, const MEDS_InstructionCheckAnnotation& p_annotation, Instruction_t *p_fallthrough)
{
	assert(getFileIR() && p_instruction);

	p_instruction->SetFallthrough(p_fallthrough);

	if (p_annotation.isUnsigned())
	{
		// use MIN_UNSIGNED
		addMovRegisterUnsignedConstant(p_instruction, p_reg, 0, p_fallthrough);
	}
	else
	{
		// treat unknown and signed the same way for overflows
		// use MIN_SIGNED for the bit width
		switch (Register::getBitWidth(p_reg))
		{
			case 64:
				addMovRegisterSignedConstant(p_instruction, p_reg, 0x8000000000000000, p_fallthrough);
				break;
			case 32:
				addMovRegisterSignedConstant(p_instruction, p_reg, 0x80000000, p_fallthrough);
				break;
			case 16:
				addMovRegisterSignedConstant(p_instruction, p_reg, 0x8000, p_fallthrough);
				break;
			case 8:
				addMovRegisterSignedConstant(p_instruction, p_reg, 0x80, p_fallthrough);
				break;
			default:
				cerr << "Transform::addMinSaturation(): invalid bit width: " << p_annotation.getBitWidth() << endl;
				break;
		}
	}
}

void Transform::setAssembly(Instruction_t *p_instr, string p_asm)
{
	m_fileIR->RegisterAssembly(p_instr, p_asm);
}

//
// Allocate and add new instruction given its assembly form
// If <p_instr> not NULL, then set fallthrough appropriately
//
// Returns the newly added instruction
//
// <p_instr> defined:
// <p_instr>            <p_instr>
// <fallthrough>  ==>   <newinstr>(<p_asm>)
//                      <fallthrough>
//
// <p_instr> is NULL:
//                ==>   <newinstr>(<p_asm>)
//
Instruction_t* Transform::addNewAssembly(Instruction_t *p_instr, string p_asm)
{
	Instruction_t* newinstr;
	if (p_instr)
		newinstr = allocateNewInstruction(p_instr->GetAddress()->GetFileID(), p_instr->GetFunction());
	else
		newinstr = allocateNewInstruction(BaseObj_t::NOT_IN_DATABASE, NULL);

	m_fileIR->RegisterAssembly(newinstr, p_asm);

	if (p_instr)
	{
		newinstr->SetFallthrough(p_instr->GetFallthrough());
		p_instr->SetFallthrough(newinstr);
	}
	else
	{
		newinstr->SetFallthrough(NULL);
	}

	return newinstr;
}

// register instruction in IRDB
// 20140421
Instruction_t* Transform::addNewAssembly(string p_asm)
{
	return addNewAssembly(NULL, p_asm);
}

// x86-64
// 20140421
void Transform::addCallbackHandler64(Instruction_t *p_orig, string p_callbackHandler, int p_numArgs)
{
	// nb: if first time, register and cache callback handler sequence
	if (m_handlerMap.count(p_callbackHandler) == 0)
	{
		m_handlerMap[p_callbackHandler] = registerCallbackHandler64(p_callbackHandler, p_numArgs);
	}

	if (p_orig)
		p_orig->SetTarget(m_handlerMap[p_callbackHandler]);
}

// x86-64
// register callback handler sequence 
//
// This following note is slightly out of date.
// We DO save flags here so that our callbacks are
// able to look at register values using a reg_values_t.
// HOWEVER, the caller of this function should still be sure
// to save flags themselves.
//
// nb: strata semantics is that it does not save flags, so we don't bother
//     saving/restoring flags either in the callback handler
//     saving/restoring flags must be done outside of this routine
// 20140416
Instruction_t* Transform::registerCallbackHandler64(string p_callbackHandler, int p_numArgs)
{
	Instruction_t *instr;
	Instruction_t *first;
	char tmpbuf[1024];

	// save flags and 16 registers (136 bytes)
	// call pushes 8 bytes
	// Total: 8 * 18 = 144
	first = instr = addNewAssembly("push rsp");
	instr = addNewAssembly(instr, "push rbp");
	instr = addNewAssembly(instr, "push rdi");
	instr = addNewAssembly(instr, "push rsi");
	instr = addNewAssembly(instr, "push rdx");
	instr = addNewAssembly(instr, "push rcx");
	instr = addNewAssembly(instr, "push rbx");
	instr = addNewAssembly(instr, "push rax");
	instr = addNewAssembly(instr, "push r8");
	instr = addNewAssembly(instr, "push r9");
	instr = addNewAssembly(instr, "push r10");
	instr = addNewAssembly(instr, "push r11");
	instr = addNewAssembly(instr, "push r12");
	instr = addNewAssembly(instr, "push r13");
	instr = addNewAssembly(instr, "push r14");
	instr = addNewAssembly(instr, "push r15");
	instr = addNewAssembly(instr, "pushf");

	// handle the arguments (if any): rdi, rsi, rdx, rcx, r8, r9
	// first arg starts at byte +144
	// now at +136??? b/c we took out the push
	instr = addNewAssembly(instr, "mov rdi, rsp");

	if (p_numArgs >= 1)
		instr = addNewAssembly(instr,  "mov rsi, [rsp+136]");
	if (p_numArgs >= 2)
		instr = addNewAssembly(instr,  "mov rdx, [rsp+144]");
	if (p_numArgs >= 3)
		instr = addNewAssembly(instr,  "mov rcx, [rsp+152]");
	if (p_numArgs >= 4)
		instr = addNewAssembly(instr,  "mov r8, [rsp+160]");
	if (p_numArgs > 4)
		assert(0); // only handle up to 5 args

	// pin the instruction that follows the callback handler
	Instruction_t* postCallback = allocateNewInstruction();
	virtual_offset_t postCallbackReturn = getAvailableAddress();
	postCallback->GetAddress()->SetVirtualOffset(postCallbackReturn);

	// push the address to return to once the callback handler is invoked
	sprintf(tmpbuf,"mov rax, 0x%x", postCallbackReturn);
	instr = addNewAssembly(instr, tmpbuf);

	instr = addNewAssembly(instr, "push rax");

	// use a nop instruction for the actual callback
	instr = addNewAssembly(instr, "nop");
	instr->SetComment(" -- callback: " + p_callbackHandler);
	instr->SetCallback(p_callbackHandler); 
	instr->SetFallthrough(postCallback); 

	// need to make sure the post callback address is pinned
	// (so that ILR and other transforms do not relocate it)
	AddressID_t *indTarg = new AddressID_t();
	m_fileIR->GetAddresses().insert(indTarg);
	indTarg->SetVirtualOffset(postCallback->GetAddress()->GetVirtualOffset());
	indTarg->SetFileID(BaseObj_t::NOT_IN_DATABASE); // SPRI global namespace
	postCallback->SetIndirectBranchTargetAddress(indTarg);

	// restore registers
	setAssembly(postCallback, "popf");           
	instr = addNewAssembly(postCallback, "pop r15");
	instr = addNewAssembly(instr, "pop r14");
	instr = addNewAssembly(instr, "pop r13");
	instr = addNewAssembly(instr, "pop r12");
	instr = addNewAssembly(instr, "pop r11");
	instr = addNewAssembly(instr, "pop r10");
	instr = addNewAssembly(instr, "pop r9");
	instr = addNewAssembly(instr, "pop r8");
	instr = addNewAssembly(instr, "pop rax");
	instr = addNewAssembly(instr, "pop rbx");
	instr = addNewAssembly(instr, "pop rcx");
	instr = addNewAssembly(instr, "pop rdx");
	instr = addNewAssembly(instr, "pop rsi");
	instr = addNewAssembly(instr, "pop rdi");
	instr = addNewAssembly(instr, "pop rbp");
	instr = addNewAssembly(instr, "lea rsp, [rsp+8]");

	instr = addNewAssembly(instr, "ret");

	// return first instruction in the callback handler chain
	return first;
}

void Transform::logMessage(const std::string &p_method, const std::string &p_msg)
{
	std::cerr << p_method << ": " << p_msg << std::endl;
}

void Transform::logMessage(const std::string &p_method, const MEDS_InstructionCheckAnnotation& p_annotation, const std::string &p_msg)
{
	logMessage(p_method, p_msg + " annotation: " + p_annotation.toString());
}

void libTransform::convertToLowercase(string &str)
{
	for (int i = 0; i < str.length(); ++i)
	{
		str[i] = tolower(str[i]);
	}
}
