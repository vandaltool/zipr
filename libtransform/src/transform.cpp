#include "transform.hpp"

#define OPTIMIZE_ASSEMBLY

using namespace libTransform;
using namespace MEDS_Annotation;

// 20130415 Anh added support for additional registers for various utility functions
// 20130415 Anh added assert() statements for unhandled registers

Transform::Transform(VariantID_t *p_variantID, FileIR_t *p_fileIR, std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *p_annotations, set<std::string> *p_filteredFunctions)
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

//
//  Before:                              After:
//  <p_instrumented> ["mov edx,1"]       <p_newInstr> []
//                                       <dupInstr>   ["mov edx,1"] <-- returns
//
Instruction_t* Transform::carefullyInsertBefore(Instruction_t* &p_instrumented, Instruction_t * &p_newInstr)
{
	db_id_t fileID = p_instrumented->GetAddress()->GetFileID();
	Function_t* func = p_instrumented->GetFunction();

	assert(p_instrumented && p_newInstr && p_instrumented->GetAddress());

	// duplicate old instrumented instruction
	Instruction_t* dupInstr = allocateNewInstruction(p_instrumented->GetAddress()->GetFileID(), p_instrumented->GetFunction());
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

	// replace instrumented with new instruction
	p_instrumented->SetDataBits(p_newInstr->GetDataBits());
	p_instrumented->SetComment(p_newInstr->GetComment());
	p_instrumented->SetCallback(p_newInstr->GetCallback());
	p_instrumented->SetFallthrough(dupInstr);

	p_instrumented->SetOriginalAddressID(BaseObj_t::NOT_IN_DATABASE);
	p_instrumented->SetIndirectBranchTargetAddress(saveIBTA);

   	p_newInstr = p_instrumented;

	return dupInstr;
}

void Transform::addPushRegister(Instruction_t *p_instr, Register::RegisterName p_reg, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(1);

	if (p_reg == Register::EAX || p_reg == Register::RAX)
	{
		dataBits[0] = 0x50; 
	} 
	else if (p_reg == Register::EBX || p_reg == Register::RBX)
	{
		dataBits[0] = 0x53; 
	} 
	else if (p_reg == Register::ECX || p_reg == Register::RCX)
	{
		dataBits[0] = 0x51; 
	}
	else if (p_reg == Register::EDX || p_reg == Register::RDX)
	{
		dataBits[0] = 0x52; 
	}
	else if (p_reg == Register::ESI || p_reg == Register::RSI)
	{
		dataBits[0] = 0x56; 
	}
	else if (p_reg == Register::EDI || p_reg == Register::RDI)
	{
		dataBits[0] = 0x57; 
	}
	else if (p_reg == Register::EBP || p_reg == Register::RBP)
	{
		dataBits[0] = 0x55; 
	}
	else if (p_reg == Register::ESP || p_reg == Register::RSP)
	{
		dataBits[0] = 0x54; 
	}
	else
	{
		dataBits.resize(2);
		dataBits[0] = 0x41;
		if (p_reg == Register::R8)
		{
			dataBits[1] = 0x50;
		} 
		else if (p_reg == Register::R9)
		{
			dataBits[1] = 0x51;
		} 
		else if (p_reg == Register::R10)
		{
			dataBits[1] = 0x52;
		} 
		else if (p_reg == Register::R11)
		{
			dataBits[1] = 0x53;
		} 
		else if (p_reg == Register::R12)
		{
			dataBits[1] = 0x54;
		} 
		else if (p_reg == Register::R13)
		{
			dataBits[1] = 0x55;
		} 
		else if (p_reg == Register::R14)
		{
			dataBits[1] = 0x56;
		} 
		else if (p_reg == Register::R15)
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

void Transform::addPopRegister(Instruction_t *p_instr, Register::RegisterName p_reg, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(1);

	if (p_reg == Register::EAX || p_reg == Register::RAX)
	{
		dataBits[0] = 0x58; 
	} 
	else if (p_reg == Register::EBX || p_reg == Register::RBX)
	{
		dataBits[0] = 0x5b; 
	} 
	else if (p_reg == Register::ECX || p_reg == Register::RCX)
	{
		dataBits[0] = 0x59; 
	}
	else if (p_reg == Register::EDX || p_reg == Register::RDX)
	{
		dataBits[0] = 0x5a; 
	}
	else if (p_reg == Register::ESI || p_reg == Register::RSI)
	{
		dataBits[0] = 0x5e; 
	}
	else if (p_reg == Register::EDI || p_reg == Register::RDI)
	{
		dataBits[0] = 0x5f; 
	}
	else if (p_reg == Register::EBP || p_reg == Register::RBP)
	{
		dataBits[0] = 0x5d; 
	}
	else if (p_reg == Register::ESP || p_reg == Register::RSP)
	{
		dataBits[0] = 0x5c; 
	}
	else
	{
		dataBits.resize(2);
		dataBits[0] = 0x41;
		if (p_reg == Register::R8)
		{
			dataBits[1] = 0x58;
		} 
		else if (p_reg == Register::R9)
		{
			dataBits[1] = 0x59;
		} 
		else if (p_reg == Register::R10)
		{
			dataBits[1] = 0x5a;
		} 
		else if (p_reg == Register::R11)
		{
			dataBits[1] = 0x5b;
		} 
		else if (p_reg == Register::R12)
		{
			dataBits[1] = 0x5c;
		} 
		else if (p_reg == Register::R13)
		{
			dataBits[1] = 0x5d;
		} 
		else if (p_reg == Register::R14)
		{
			dataBits[1] = 0x5e;
		} 
		else if (p_reg == Register::R15)
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

void Transform::addPushf(Instruction_t *p_pushf_i, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(1);
	dataBits[0] = 0x9c;
	addInstruction(p_pushf_i, dataBits, p_fallThrough, NULL);
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
	AddressID_t *a =new AddressID_t();

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

// returns true if BeaEngine says arg1 of the instruction is a register 
bool Transform::hasTargetRegister(Instruction_t *p_instruction)
{
	if (!p_instruction)
		return false;

	DISASM disasm;
	p_instruction->Disassemble(disasm);
	
	return disasm.Argument1.ArgType & 0xFFFF0000 & REGISTER_TYPE;
}

Register::RegisterName Transform::getTargetRegister(Instruction_t *p_instruction)
{
	if (hasTargetRegister(p_instruction))
	{
		DISASM disasm;
		p_instruction->Disassemble(disasm);

		return Register::getRegister(disasm.Argument1.ArgMnemonic);
	}
	else
		return Register::UNKNOWN;
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
		return strcasestr(assembly.c_str(), "MUL") != NULL;
	}

	DISASM disasm;
	p_instruction->Disassemble(disasm);

	// beaengine adds space at the end of the mnemonic string
	return strcasestr(disasm.Instruction.Mnemonic, "MUL ") != NULL;
}

//
// Returns true iff instruction is ADD or SUB (according to BeaEngine)
//
bool Transform::isAddSubNonEspInstruction(Instruction_t *p_instruction)
{
	if (!p_instruction)
		return false;

	DISASM disasm;

	p_instruction->Disassemble(disasm);

	// beaengine adds space at the end of the mnemonic string
	if (strcasestr(disasm.Instruction.Mnemonic, "ADD "))
	{
		return true;
	}
	else if (strcasestr(disasm.Instruction.Mnemonic, "SUB ")) 
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

void Transform::addTestRegister8(Instruction_t *p_instr, Register::RegisterName p_reg, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(2);
	if (p_reg == Register::AL)
	{
		dataBits[0] = 0x84;
		dataBits[1] = 0xc0;
	}
	else if (p_reg == Register::BL)
	{
		dataBits[0] = 0x84;
		dataBits[1] = 0xdb;
	}
	else if (p_reg == Register::CL)
	{
		dataBits[0] = 0x84;
		dataBits[1] = 0xc9;
	}
	else if (p_reg == Register::DL)
	{
		dataBits[0] = 0x84;
		dataBits[1] = 0xd2;
	}
	else if (p_reg == Register::AH)
	{
		dataBits[0] = 0x84;
		dataBits[1] = 0xe4;
	}
	else if (p_reg == Register::BH)
	{
		dataBits[0] = 0x84;
		dataBits[1] = 0xff;
	}
	else if (p_reg == Register::CH)
	{
		dataBits[0] = 0x84;
		dataBits[1] = 0xed;
	}
	else if (p_reg == Register::DH)
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

void Transform::addTestRegister16(Instruction_t *p_instr, Register::RegisterName p_reg, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(3);
	if (p_reg == Register::AX)
	{
		dataBits[0] = 0x66;
		dataBits[1] = 0x85;
		dataBits[2] = 0xc0;
	}
	else if (p_reg == Register::BX)
	{
		dataBits[0] = 0x66;
		dataBits[1] = 0x85;
		dataBits[2] = 0xdb;
	}
	else if (p_reg == Register::CX)
	{
		dataBits[0] = 0x66;
		dataBits[1] = 0x85;
		dataBits[2] = 0xc9;
	}
	else if (p_reg == Register::DX)
	{
		dataBits[0] = 0x66;
		dataBits[1] = 0x85;
		dataBits[2] = 0xd2;
	}
	else if (p_reg == Register::BP)
	{
		assert(0);
	}
	else if (p_reg == Register::SP)
	{
		assert(0);
	}
	else if (p_reg == Register::SI)
	{
		assert(0);
	}
	else if (p_reg == Register::DI)
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
void Transform::addTestRegister32(Instruction_t *p_instr, Register::RegisterName p_reg, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(2);
	if (p_reg == Register::EAX)
	{
		dataBits[0] = 0x85;
		dataBits[1] = 0xc0;
	}
	else if (p_reg == Register::EBX)
	{
		dataBits[0] = 0x85;
		dataBits[1] = 0xdb;
	}
	else if (p_reg == Register::ECX)
	{
		dataBits[0] = 0x85;
		dataBits[1] = 0xc9;
	}
	else if (p_reg == Register::EDX)
	{
		dataBits[0] = 0x85;
		dataBits[1] = 0xd2;
	}
	else if (p_reg == Register::ESI)
	{
		dataBits[0] = 0x85;
		dataBits[1] = 0xf6;
	}
	else if (p_reg == Register::EDI)
	{
		dataBits[0] = 0x85;
		dataBits[1] = 0xff;
	}
	else if (p_reg == Register::EBP)
	{
		dataBits[0] = 0x85;
		dataBits[1] = 0xed;
	}
	else if (p_reg == Register::ESP)
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


void Transform::addTestRegister(Instruction_t *p_instr, Register::RegisterName p_reg, Instruction_t *p_fallThrough)
{
	if (Register::is8bit(p_reg))
		addTestRegister8(p_instr, p_reg, p_fallThrough);
	else if (Register::is16bit(p_reg))
		addTestRegister16(p_instr, p_reg, p_fallThrough);
	else if (Register::is32bit(p_reg))
		addTestRegister32(p_instr, p_reg, p_fallThrough);
}

void Transform::addTestRegisterMask(Instruction_t *p_instr, Register::RegisterName p_reg, unsigned p_mask, Instruction_t *p_fallThrough)
{
	if (Register::is32bit(p_reg))
		addTestRegisterMask32(p_instr, p_reg, p_mask, p_fallThrough);
}

void Transform::addTestRegisterMask32(Instruction_t *p_instr, Register::RegisterName p_reg, unsigned p_mask, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(6); // all but EAX take 6 bytes
	unsigned *tmp;

	if (p_reg == Register::EAX)
	{
		dataBits.resize(5);
		dataBits[0] = 0xa9;
		tmp = (unsigned *) &dataBits[1];
		*tmp = p_mask;
	}
	else if (p_reg == Register::EBX)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xc3;
		tmp = (unsigned *) &dataBits[2];
		*tmp = p_mask;
	}
	else if (p_reg == Register::ECX)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xc1;
		tmp = (unsigned *) &dataBits[2];
		*tmp = p_mask;
	}
	else if (p_reg == Register::EDX)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xc2;
		tmp = (unsigned *) &dataBits[2];
		*tmp = p_mask;
	}
	else if (p_reg == Register::ESI)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xc6;
		tmp = (unsigned *) &dataBits[2];
		*tmp = p_mask;
	}
	else if (p_reg == Register::EDI)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xc7;
		tmp = (unsigned *) &dataBits[2];
		*tmp = p_mask;
	}
	else if (p_reg == Register::EBP)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xc5;
		tmp = (unsigned *) &dataBits[2];
		*tmp = p_mask;
	}
	else if (p_reg == Register::ESP)
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

void Transform::addCmpRegisterMask(Instruction_t *p_instr, Register::RegisterName p_reg, unsigned p_mask, Instruction_t *p_fallThrough)
{
	if (Register::is32bit(p_reg))
		addCmpRegisterMask32(p_instr, p_reg, p_mask, p_fallThrough);
}

void Transform::addCmpRegisterMask32(Instruction_t *p_instr, Register::RegisterName p_reg, unsigned p_mask, Instruction_t *p_fallThrough)
{
	string dataBits;
	unsigned *tmp;

	if (p_reg == Register::EAX)
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
     if (p_reg == Register::EBX)
	  {
       dataBits[1] = 0xfb;
     }
     else if (p_reg == Register::ECX)
     {
       dataBits[1] = 0xf9;
     }
     else if (p_reg == Register::EDX)
     {
       dataBits[1] = 0xfa;
     }
     else if (p_reg == Register::EBP)
     {
       dataBits[1] = 0xfd;
     }
     else if (p_reg == Register::ESI)
     {
       dataBits[1] = 0xfe;
     }
     else if (p_reg == Register::EDI)
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
void Transform::addNot(Instruction_t *p_instr, Register::RegisterName p_reg, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(2); 

	if (p_reg == Register::EAX)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xd0;
	}
	else if (p_reg == Register::EBX)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xd3;
	}
	else if (p_reg == Register::ECX)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xd1;
	}
	else if (p_reg == Register::EDX)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xd2;
	}
	else if (p_reg == Register::ESI)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xd6;
	}
	else if (p_reg == Register::EDI)
	{
		dataBits[0] = 0xf7;
		dataBits[1] = 0xd7;
	}
	else if (p_reg == Register::AX)
	{
		dataBits.resize(3); 
		dataBits[0] = 0x66;
		dataBits[1] = 0xf7;
		dataBits[2] = 0xd0;
	}
	else if (p_reg == Register::BX)
	{
		dataBits.resize(3); 
		dataBits[0] = 0x66;
		dataBits[1] = 0xf7;
		dataBits[2] = 0xd3;
	}
	else if (p_reg == Register::CX)
	{
		dataBits.resize(3); 
		dataBits[0] = 0x66;
		dataBits[1] = 0xf7;
		dataBits[2] = 0xd1;
	}
	else if (p_reg == Register::DX)
	{
		dataBits.resize(3); 
		dataBits[0] = 0x66;
		dataBits[1] = 0xf7;
		dataBits[2] = 0xd2;
	}
	else if (p_reg == Register::AL)
	{
		dataBits[0] = 0xf6;
		dataBits[1] = 0xd0;
	}
	else if (p_reg == Register::BL)
	{
		dataBits[0] = 0xf6;
		dataBits[1] = 0xd3;
	}
	else if (p_reg == Register::CL)
	{
		dataBits[0] = 0xf6;
		dataBits[1] = 0xd1;
	}
	else if (p_reg == Register::DL)
	{
		dataBits[0] = 0xf6;
		dataBits[1] = 0xd2;
	}
	else if (p_reg == Register::AH)
	{
		dataBits[0] = 0xf6;
		dataBits[1] = 0xd4;
	}
	else if (p_reg == Register::BH)
	{
		dataBits[0] = 0xf6;
		dataBits[1] = 0xd7;
	}
	else if (p_reg == Register::CH)
	{
		dataBits[0] = 0xf6;
		dataBits[1] = 0xd5;
	}
	else if (p_reg == Register::DH)
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
void Transform::addAddRegisters(Instruction_t *p_instr, Register::RegisterName p_regTgt, Register::RegisterName p_regSrc, Instruction_t *p_fallThrough)
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
void Transform::addAddRegisterConstant(Instruction_t *p_instr, Register::RegisterName p_reg, int p_constantValue, Instruction_t *p_fallThrough)
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
void Transform::addMulRegisterConstant(Instruction_t *p_instr, Register::RegisterName p_reg, int p_constantValue, Instruction_t *p_fallThrough)
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
void Transform::addMovRegisters(Instruction_t *p_instr, Register::RegisterName p_regTgt, Register::RegisterName p_regSrc, Instruction_t *p_fallThrough)
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

void Transform::addMovRegisterSignedConstant(Instruction_t *p_instr, Register::RegisterName p_regTgt, long int p_constant, Instruction_t *p_fallThrough)
{
	p_instr->SetFallthrough(p_fallThrough);

	char buf[128];

	sprintf(buf,"mov %s, %ld", Register::toString(p_regTgt).c_str(), p_constant);

	string assembly(buf);
	m_fileIR->RegisterAssembly(p_instr, assembly);

	p_instr->SetComment("Saturating arithmetic");
}

void Transform::addMovRegisterUnsignedConstant(Instruction_t *p_instr, Register::RegisterName p_regTgt, unsigned long int p_constant, Instruction_t *p_fallThrough)
{
	p_instr->SetFallthrough(p_fallThrough);

	char buf[128];
	sprintf(buf,"mov %s, %lu", Register::toString(p_regTgt).c_str(), p_constant);

	string assembly(buf);
	m_fileIR->RegisterAssembly(p_instr, assembly);

	p_instr->SetComment("Saturating arithmetic");
}

void Transform::addAndRegister32Mask(Instruction_t *p_instr, Register::RegisterName p_regTgt, unsigned int p_mask, Instruction_t *p_fallThrough)
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

// known to be used on x86-64

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
#ifdef OLD_WAY
	string dataBits;
	dataBits.resize(2);
	dataBits[0] = 0x71;
	dataBits[1] = 0x00; // value doesn't matter -- we will fill it in later

	addInstruction(p_instr, dataBits, p_fallThrough, p_target);
#endif

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

void Transform::addMaxSaturation(Instruction_t *p_instruction, Register::RegisterName p_reg, const MEDS_InstructionCheckAnnotation& p_annotation, Instruction_t *p_fallthrough)
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
void Transform::addMinSaturation(Instruction_t *p_instruction, Register::RegisterName p_reg, const MEDS_InstructionCheckAnnotation& p_annotation, Instruction_t *p_fallthrough)
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

