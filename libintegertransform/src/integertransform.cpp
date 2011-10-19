#include "integertransform.hpp"

IntegerTransform::IntegerTransform(VariantID_t *p_variantID, VariantIR_t *p_variantIR, std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *p_annotations, set<std::string> *p_filteredFunctions)
{
	m_variantID = p_variantID;                  // Current variant ID
	m_variantIR = p_variantIR;                  // IR (off the database) for variant
	m_annotations = p_annotations;              // MEDS annotations
	m_filteredFunctions = p_filteredFunctions;  // Blacklisted funtions
}

// iterate through all functions
// filter those functions that should be ignored
//    iterate through all instructions in function
//    if MEDS annotation says to instrument
//       add instrumentation
int IntegerTransform::execute()
{
	for(
	  set<Function_t*>::const_iterator itf=m_variantIR->GetFunctions().begin();
	  itf!=m_variantIR->GetFunctions().end();
	  ++itf
	  )
	{
		Function_t* func=*itf;

		if (m_filteredFunctions->find(func->GetName()) != m_filteredFunctions->end())
			continue;

		for(
		  set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
		  it!=func->GetInstructions().end();
		  ++it)
		{
			Instruction_t* insn=*it;

			// lookup address
			// see if there's an annotation
			//   if so perform the instrumentation check

		} // end iterate over all instructions in a function
	} // end iterate over all functions

	m_variantIR->WriteToDB();

	// for now just be happy
	return 0;
}

//
//      jno <originalFallthroughInstruction>
//      pusha
//      pushf
//      push_arg
//      push L1
//      ... setup detector ...
//  L1: pop_arg
//      popf
//      popa
//
void IntegerTransform::addOverflowCheck(Instruction_t *p_instruction, std::string p_detector)
{
	assert(m_variantIR && p_instruction);
	
	string dataBits;

	AddressID_t *jno_a =new AddressID_t;
	AddressID_t *jo_a =new AddressID_t;
	AddressID_t *jmporigfallthrough_a =new AddressID_t;
	AddressID_t *pusha_a =new AddressID_t;
	AddressID_t *pushf_a =new AddressID_t;
	AddressID_t *pusharg_a =new AddressID_t;
	AddressID_t *pushret_a =new AddressID_t;
	AddressID_t *poparg_a =new AddressID_t;
	AddressID_t *popf_a =new AddressID_t;
	AddressID_t *popa_a =new AddressID_t;

	jno_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	jmporigfallthrough_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	pusha_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	pushf_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	pusharg_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	pushret_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	poparg_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	popf_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	popa_a->SetFileID(p_instruction->GetAddress()->GetFileID());

	Instruction_t* jno_i = new Instruction_t;
	Instruction_t* jmporigfallthrough_i = new Instruction_t;
	Instruction_t* pusha_i = new Instruction_t;
	Instruction_t* pushf_i = new Instruction_t;
	Instruction_t* pusharg_i = new Instruction_t;
	Instruction_t* pushret_i = new Instruction_t;
	Instruction_t* poparg_i = new Instruction_t;
	Instruction_t* popf_i = new Instruction_t;
	Instruction_t* popa_i = new Instruction_t;

	Function_t* origFunction = p_instruction->GetFunction();

	jno_i->SetFunction(origFunction);
	jmporigfallthrough_i->SetFunction(origFunction);
	pusha_i->SetFunction(origFunction);
	pushf_i->SetFunction(origFunction);
	pusharg_i->SetFunction(origFunction);
	pushret_i->SetFunction(origFunction);
	poparg_i->SetFunction(origFunction);
	popf_i->SetFunction(origFunction);
	popa_i->SetFunction(origFunction);

	// pin the poparg instruction 
	virtual_offset_t postDetectorReturn = getAvailableAddress(m_variantIR);
	poparg_a->SetVirtualOffset(postDetectorReturn);

        jno_i->SetAddress(jno_a);
        jmporigfallthrough_i->SetAddress(jmporigfallthrough_a);
        pusha_i->SetAddress(pusha_a);
        pushf_i->SetAddress(pushf_a);
        pusharg_i->SetAddress(pusharg_a);
        pushret_i->SetAddress(pushret_a);
        poparg_i->SetAddress(poparg_a);
        popf_i->SetAddress(popf_a);
        popa_i->SetAddress(popa_a);

        // set fallthrough for the original instruction
	Instruction_t* nextOrig_i = p_instruction->GetFallthrough();
        p_instruction->SetFallthrough(jno_i); 

        // jno IO
        dataBits.resize(2);
        dataBits[0] = 0x71;
        dataBits[1] = 0x15; // value doesn't matter, we will fill it in later
        jno_i->SetDataBits(dataBits);
        jno_i->SetComment(jno_i->getDisassembly());
	jno_i->SetFallthrough(pusha_i); 
	jno_i->SetTarget(nextOrig_i); 

        // pusha   
        dataBits.resize(1);
        dataBits[0] = 0x60;
        pusha_i->SetDataBits(dataBits);
        pusha_i->SetComment(pusha_i->getDisassembly());
	pusha_i->SetFallthrough(pusharg_i); 

        // pushf   
        dataBits.resize(1);
        dataBits[0] = 0x9c;
        pushf_i->SetDataBits(dataBits);
        pushf_i->SetComment(pushf_i->getDisassembly());
	pushf_i->SetFallthrough(pusharg_i); 

        // push arg
        dataBits.resize(5);
        dataBits[0] = 0x68;
	virtual_offset_t *tmp = (virtual_offset_t *) &dataBits[1];
	*tmp = p_instruction->GetAddress()->GetVirtualOffset();
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
	poparg_i->SetFallthrough(popa_i); 
	poparg_i->SetIndirectBranchTargetAddress(poparg_a);  
	poparg_i->SetCallback(p_detector); 

        // popf   
        dataBits.resize(1);
        dataBits[0] = 0x9d;
        popf_i->SetDataBits(dataBits);
        popf_i->SetComment(popf_i->getDisassembly());
	popf_i->SetFallthrough(popa_i); 
//	popf_i->SetCallback("integer_overflow_detector"); 

        // popa   
        dataBits.resize(1);
        dataBits[0] = 0x61;
        popa_i->SetDataBits(dataBits);
        popa_i->SetComment(popa_i->getDisassembly());
	popa_i->SetFallthrough(nextOrig_i); 

        // add new address to IR
	m_variantIR->GetAddresses().insert(jno_a);
	m_variantIR->GetAddresses().insert(jmporigfallthrough_a);
	m_variantIR->GetAddresses().insert(pusha_a);
	m_variantIR->GetAddresses().insert(pusharg_a);
	m_variantIR->GetAddresses().insert(pushf_a);
	m_variantIR->GetAddresses().insert(pushret_a);
	m_variantIR->GetAddresses().insert(popf_a);
	m_variantIR->GetAddresses().insert(poparg_a);
	m_variantIR->GetAddresses().insert(popa_a);

        // add new instructions to IR
	m_variantIR->GetInstructions().insert(jno_i);
	m_variantIR->GetInstructions().insert(jmporigfallthrough_i);
	m_variantIR->GetInstructions().insert(pusha_i);
	m_variantIR->GetInstructions().insert(pusharg_i);
	m_variantIR->GetInstructions().insert(pushf_i);
	m_variantIR->GetInstructions().insert(pushret_i);
	m_variantIR->GetInstructions().insert(popf_i);
	m_variantIR->GetInstructions().insert(poparg_i);
	m_variantIR->GetInstructions().insert(popa_i);
}

virtual_offset_t IntegerTransform::getAvailableAddress(VariantIR_t *p_virp)
{
	// traverse all instructions
	// grab address
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

	// @todo: lookup instruction size so that we don't waste any space
	// for some reason the max available address is incorrect! was ist los?
static int counter = -16;
        counter += 16;
	return 0xf0000000 + counter;
// availableAddressOffset + 16;
}

#ifdef NOTYET

//
// Returns true iff instruction is mul or imul
//
static bool isMultiplyInstruction32(Instruction_t *p_instruction)
{
	if (!p_instruction)
		return false;

	DISASM disasm;

	p_instruction->Disassemble(disasm);

	// look for "mul ..." or "imul ..."
	// beaengine adds space at the end of the mnemonic string
	return strcasestr(disasm.Instruction.Mnemonic, "mul ") != NULL;
}

//
// Returns true iff instruction is mul or imul
//
static bool isAddSubNonEspInstruction32(Instruction_t *p_instruction)
{
	if (!p_instruction)
		return false;

	DISASM disasm;

	// look for "add ..." or "sub ..."
	// look for "addl ..." or "subl ..."
	p_instruction->Disassemble(disasm);

//fprintf(stderr,"INT DEBUG: inst: 0x%x [%s] [%s]\n", p_instruction->GetAddress(), disasm.Instruction.Mnemonic, p_instruction->GetComment().c_str());

	// beaengine adds space at the end of the mnemonic string
	if (strcasestr(disasm.Instruction.Mnemonic, "add "))
	{
		return true;
	}
	else if (strcasestr(disasm.Instruction.Mnemonic, "sub ")) 
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
#endif
