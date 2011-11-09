#include "transform.hpp"

using namespace libTransform;
using namespace MEDS_Annotation;

Transform::Transform(VariantID_t *p_variantID, VariantIR_t *p_variantIR, std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *p_annotations, set<std::string> *p_filteredFunctions)
{
	m_variantID = p_variantID;                  // Current variant ID
	m_variantIR = p_variantIR;                  // IR (off the database) for variant
	m_annotations = p_annotations;              // MEDS annotations
	m_filteredFunctions = p_filteredFunctions;  // Blacklisted funtions
}

void Transform::addInstruction(Instruction_t *p_instr, string p_dataBits, Instruction_t *p_fallThrough, Instruction_t *p_target)
{
	if (p_instr == NULL) return;

	p_instr->SetDataBits(p_dataBits);
	p_instr->SetComment(p_instr->getDisassembly());
	p_instr->SetFallthrough(p_fallThrough); 
	p_instr->SetTarget(p_target); 

cerr << "Transform::addInstruction: " << p_instr->GetComment() << endl;

	m_variantIR->GetAddresses().insert(p_instr->GetAddress());
	m_variantIR->GetInstructions().insert(p_instr);
}

Instruction_t* Transform::carefullyInsertBefore(Instruction_t* &p_targetInstr, Instruction_t * &p_newInstr)
{
	db_id_t fileID = p_targetInstr->GetAddress()->GetFileID();
	Function_t* func = p_targetInstr->GetFunction();

	assert(p_targetInstr && p_newInstr && p_targetInstr->GetAddress());

	// duplicate old instruction
	Instruction_t* dupInstr = allocateNewInstruction(p_targetInstr->GetAddress()->GetFileID(), p_targetInstr->GetFunction());
	dupInstr->SetDataBits(p_targetInstr->GetDataBits());
	dupInstr->SetComment(p_targetInstr->GetComment());
	dupInstr->SetCallback(p_targetInstr->GetCallback());
	dupInstr->SetFallthrough(p_targetInstr->GetFallthrough());
	dupInstr->SetTarget(p_targetInstr->GetTarget());

	// replace old with new instruction
	p_targetInstr->SetDataBits(p_newInstr->GetDataBits());
	p_targetInstr->SetComment(p_newInstr->GetComment());
	p_targetInstr->SetCallback(p_newInstr->GetCallback());
	p_targetInstr->SetFallthrough(dupInstr);
	p_targetInstr->SetIndirectBranchTargetAddress(dupInstr->GetIndirectBranchTargetAddress());

	// clear indirect branch target address of duplicated old instruction
	dupInstr->SetIndirectBranchTargetAddress(NULL);

    p_newInstr = p_targetInstr;
	return dupInstr;
}

void Transform::addPushRegister(Instruction_t *p_instr, Register::RegisterName p_reg, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(1);

	if (p_reg == Register::EAX)
	{
		dataBits[0] = 0x50; 
	} 
	else if (p_reg == Register::EBX)
	{
		dataBits[0] = 0x53; 
	} 
	else if (p_reg == Register::ECX)
	{
		dataBits[0] = 0x51; 
	}
	else if (p_reg == Register::EDX)
	{
		dataBits[0] = 0x52; 
	}
	else
	{
		return;
	}

	addInstruction(p_instr, dataBits, p_fallThrough, NULL);
}

void Transform::addPopRegister(Instruction_t *p_instr, Register::RegisterName p_reg, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(1);

	if (p_reg == Register::EAX)
	{
		dataBits[0] = 0x58; 
	} 
	else if (p_reg == Register::EBX)
	{
		dataBits[0] = 0x5b; 
	} 
	else if (p_reg == Register::ECX)
	{
		dataBits[0] = 0x59; 
	}
	else if (p_reg == Register::EDX)
	{
		dataBits[0] = 0x5a; 
	}
	else
	{
		return;
	}

	return addInstruction(p_instr, dataBits, p_fallThrough, NULL);
}

void Transform::addPusha(Instruction_t *p_pusha_i, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(1);
	dataBits[0] = 0x60;
	return addInstruction(p_pusha_i, dataBits, p_fallThrough, NULL);
}

void Transform::addPushf(Instruction_t *p_pushf_i, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(1);
	dataBits[0] = 0x9c;
	return addInstruction(p_pushf_i, dataBits, p_fallThrough, NULL);
}

void Transform::addPopf(Instruction_t *p_popf_i, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(1);
	dataBits[0] = 0x9d;
	return addInstruction(p_popf_i, dataBits, p_fallThrough, NULL);
}

void Transform::addPopa(Instruction_t *p_popa_i, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(1);
	dataBits[0] = 0x61;
	return addInstruction(p_popa_i, dataBits, p_fallThrough, NULL);
}

void Transform::addNop(Instruction_t *p_nop_i, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(1);
	dataBits[0] = 0x90;
	return addInstruction(p_nop_i, dataBits, p_fallThrough, NULL);
}

Instruction_t* Transform::allocateNewInstruction(db_id_t p_fileID, Function_t* p_func)
{
	Instruction_t *instr = new Instruction_t();
	AddressID_t *a =new AddressID_t();

	a->SetFileID(p_fileID);

	instr->SetFunction(p_func);
	instr->SetAddress(a);

	m_variantIR->GetInstructions().insert(instr);
	m_variantIR->GetAddresses().insert(a);
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

void Transform::addCallbackHandler(string p_detector, Instruction_t *p_instrumentedInstruction, Instruction_t *p_instruction, Instruction_t *p_fallThrough)
{
cerr << "void Transform::addCallbackHandler(): enter: " << p_instruction->GetComment() << endl;
	assert(getVariantIR() && p_instruction);
	
	string dataBits;

	db_id_t fileID = p_instruction->GetAddress()->GetFileID();
	Function_t* func = p_instruction->GetFunction();

 	// create and register new instructions (and addresses)
	Instruction_t* pusha_i = allocateNewInstruction(fileID, func);
	Instruction_t* pushf_i = allocateNewInstruction(fileID, func);
	Instruction_t* pusharg_i = allocateNewInstruction(fileID, func);
	Instruction_t* pushret_i = allocateNewInstruction(fileID, func);
	Instruction_t* poparg_i = allocateNewInstruction(fileID, func);
	Instruction_t* popf_i = allocateNewInstruction(fileID, func);
	Instruction_t* popa_i = allocateNewInstruction(fileID, func);

	// pin the poparg instruction 
	virtual_offset_t postDetectorReturn = getAvailableAddress();
	poparg_i->GetAddress()->SetVirtualOffset(postDetectorReturn);

	// link callback handler sequence to instrumented instruction
	p_instruction->SetFallthrough(pusha_i);
	p_instruction->SetComment(p_instruction->GetComment() + " -- Link to callback handler sequence");

	// pusha   
	addPusha(pusha_i, pushf_i);
	pusha_i->SetComment(pusha_i->GetComment() + " -- start of callback handler sequence");

	// pushf   
	addPushf(pushf_i, pusharg_i);

	// push (PC of instrumented instruction)
	dataBits.resize(5);
	dataBits[0] = 0x68;
	virtual_offset_t *tmp = (virtual_offset_t *) &dataBits[1];
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
	poparg_i->SetFallthrough(popf_i); 
	poparg_i->SetIndirectBranchTargetAddress(poparg_i->GetAddress());  
	poparg_i->SetCallback(p_detector); 

	// popf   
	addPopf(popf_i, popa_i);

	// popa   
	addPopa(popa_i, p_fallThrough);

cerr << "void Transform::addCallbackHandler(): exit" << endl;
}


//
// Returns true iff instruction is MUL (according to BeaEngine)
//
bool Transform::isMultiplyInstruction(Instruction_t *p_instruction)
{
	if (!p_instruction)
		return false;

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
	else
	{
		cerr << "Transform::addTestRegister8(): unhandled register" << endl;
		return;
	}

	return addInstruction(p_instr, dataBits, p_fallThrough, NULL);
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
	else
	{
		cerr << "Transform::addTestRegister16(): unhandled register" << endl;
		return;
	}

	return addInstruction(p_instr, dataBits, p_fallThrough, NULL);
}

// same as 16 bit version? hmm, weird
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
	else
	{
		cerr << "Transform::addTestRegister32(): unhandled register" << endl;
		return;
	}

	return addInstruction(p_instr, dataBits, p_fallThrough, NULL);
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


// jump not signed
void Transform::addJns(Instruction_t *p_instr, Instruction_t *p_fallThrough, Instruction_t *p_target)
{
	string dataBits;
	dataBits.resize(2);
	dataBits[0] = 0x79;
	dataBits[1] = 0x00; // value doesn't matter -- we will fill it in later
	return addInstruction(p_instr, dataBits, p_fallThrough, p_target);
}
