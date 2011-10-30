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

	m_variantIR->GetAddresses().insert(p_instr->GetAddress());
	m_variantIR->GetInstructions().insert(p_instr);
}

void Transform::addPushRegister(Instruction_t *p_instr, Register::RegisterName p_reg, Instruction_t *p_fallThrough)
{
	string dataBits;
	dataBits.resize(2);

	dataBits[0] = 0x66;
	if (p_reg == Register::EAX)
	{
		dataBits[1] = 0x50; 
	} 
	else if (p_reg == Register::EBX)
	{
		dataBits[1] = 0x53; 
	} 
	else if (p_reg == Register::ECX)
	{
		dataBits[1] = 0x51; 
	}
	else if (p_reg == Register::EDX)
	{
		dataBits[1] = 0x52; 
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
	dataBits.resize(2);

	dataBits[0] = 0x66;
	if (p_reg == Register::EAX)
	{
		dataBits[1] = 0x58; 
	} 
	else if (p_reg == Register::EBX)
	{
		dataBits[1] = 0x5b; 
	} 
	else if (p_reg == Register::ECX)
	{
		dataBits[1] = 0x59; 
	}
	else if (p_reg == Register::EDX)
	{
		dataBits[1] = 0x5a; 
	}
	else
	{
		return;
	}

	return addInstruction(p_instr, dataBits, p_fallThrough, NULL);
}

Instruction_t* Transform::allocateNewInstruction(db_id_t p_fileID, Function_t* p_func)
{
	Instruction_t *instr = new Instruction_t();
	AddressID_t *a =new AddressID_t();

	a->SetFileID(p_fileID);

	instr->SetFunction(p_func);
	instr->SetAddress(a);
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

Instruction_t* Transform::addCallbackHandler(string p_detector, Instruction_t *p_instr)
{

}
