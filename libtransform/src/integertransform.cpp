#include <assert.h>
#include "integertransform.hpp"

using namespace libTransform;

IntegerTransform::IntegerTransform(VariantID_t *p_variantID, VariantIR_t *p_variantIR, std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *p_annotations, set<std::string> *p_filteredFunctions) : Transform(p_variantID, p_variantIR, p_annotations, p_filteredFunctions) 
{
}

// iterate through all functions
// filter those functions that should be ignored
//    iterate through all instructions in function
//    if MEDS annotation says to instrument
//       add instrumentation
int IntegerTransform::execute()
{
	for(
	  set<Function_t*>::const_iterator itf=getVariantIR()->GetFunctions().begin();
	  itf!=getVariantIR()->GetFunctions().end();
	  ++itf
	  )
	{
		Function_t* func=*itf;

		cerr << "integertransform: looking at function: " << func->GetName() << endl;

		if (getFilteredFunctions()->find(func->GetName()) != getFilteredFunctions()->end())
			continue;

		for(
		  set<Instruction_t*>::const_iterator it=func->GetInstructions().begin();
		  it!=func->GetInstructions().end();
		  ++it)
		{
			Instruction_t* insn=*it;

			if (insn && insn->GetAddress())
			{
				virtual_offset_t irdb_vo = insn->GetAddress()->GetVirtualOffset();
				if (irdb_vo == 0) continue;

				VirtualOffset vo(irdb_vo);

				MEDS_InstructionCheckAnnotation annotation = (*getAnnotations())[vo];
				if (!annotation.isValid()) 
				{
					// even if no annotation but is a multiply, we instrument it
					if (isMultiplyInstruction(insn))
						handleOverflowCheck(insn, annotation);
					continue;
				}
				else if (annotation.isOverflow())
				{
					cerr << "integertransform: overflow annotation: " << annotation.toString();
					handleOverflowCheck(insn, annotation);
				}
				else if (annotation.isUnderflow())
				{
					cerr << "integertransform: underflow annotation: " << annotation.toString();
					handleOverflowCheck(insn, annotation);
				}
				else if (annotation.isTruncation())
				{
					cerr << "integertransform: truncation annotation: " << annotation.toString() << endl;
					handleTruncation(insn, annotation);

				}
				else if (annotation.isSignedness())
				{
					cerr << "integertransform: signedness annotation: " << annotation.toString() << endl;
					handleSignedness(insn, annotation);
				}
				else
					cerr << "integertransform: unknown annotation: " << annotation.toString();
			}
		} // end iterate over all instructions in a function
	} // end iterate over all functions

	getVariantIR()->WriteToDB();

	// for now just be happy
	return 0;
}

void IntegerTransform::handleSignedness(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation)
{
	if (p_annotation.getBitWidth() == 16 && p_annotation.isSigned())
	{
		cerr << "handleSignedness(): calling addSignednessCheck16" << endl;
		addSignednessCheck16(p_instruction, p_annotation);
	}
	else
		cerr << "handleSignedness(): case not yet handled" << endl;
}

//                                                                <save flags>
//                                                                TEST ax, ax
//                                                                jns L1
//                                                                invoke conversion handler
//                                                        L1:     <restore flags>
//
// 8048576      5 INSTR CHECK SIGNEDNESS SIGNED 16 AX ZZ mov     [esp+28h], ax

void IntegerTransform::addSignednessCheck16(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation)
{
	// sanity checks
    assert(getVariantIR() && p_instruction);
	assert (p_annotation.isValid() && Register::is16bit(p_annotation.getRegister()));

    db_id_t fileID = p_instruction->GetAddress()->GetFileID();
    Function_t* func = p_instruction->GetFunction();

	Instruction_t* pushf_i = allocateNewInstruction(fileID, func);
	Instruction_t* test_i = allocateNewInstruction(fileID, func);
	Instruction_t* jns_i = allocateNewInstruction(fileID, func);
	Instruction_t* nop_i = allocateNewInstruction(fileID, func);
	Instruction_t* popf_i = allocateNewInstruction(fileID, func);

	addPushf(pushf_i, test_i);
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, pushf_i);
	pushf_i->SetFallthrough(test_i); // do I need this here again b/c carefullyInsertBefore breaks the link?
	addTestRegister(test_i, p_annotation.getRegister(), jns_i);
	addJns(jns_i, nop_i, popf_i);
	addNop(nop_i, popf_i);
	addCallbackHandler(string(SIGNEDNESS_DETECTOR_16), originalInstrumentInstr, nop_i, popf_i);
	addPopf(popf_i, originalInstrumentInstr);
}

void IntegerTransform::handleOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation)
{
	if (isMultiplyInstruction(p_instruction))
	{
		addOverflowCheck32(p_instruction, p_annotation);
	}
	else if (p_annotation.isUnderflow() || p_annotation.isOverflow())
	{
		addOverflowCheck32(p_instruction, p_annotation);
	/*	
		if (p_annotation.getBitWidth() == 16)
		{
			if (p_annotation.isSigned())
			{
				if (isAddSubNonEspInstruction(p_instruction))
				{
					addAddSubSignedOverflowCheck16(p_instruction, p_annotation);
				}
			}
		}
		else if (p_annotation.getBitWidth() == 32)
		{
			if (p_annotation.isSigned() || p_annotation.isUnsigned())
			{
				addOverflowCheck32(p_instruction, p_annotation);
			}
			else
			{
				// call anyways, just assume it's signed
				addOverflowCheck32(p_instruction, p_annotation);
			}
		}
		*/
	}
}

void IntegerTransform::handleTruncation(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation)
{
	if (p_annotation.getTruncationFromWidth() == 32 && p_annotation.getTruncationToWidth() == 16)
	{
		addTruncationCheck32to16(p_instruction, p_annotation);
	}
	else
	{
		cerr << "integertransform: TRUNCATION annotation not yet handled: " << p_annotation.toString() << endl;
	}
}

void IntegerTransform::addTruncationCheck32to16(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation)
{
	string detector; // name of SPRI/STRATA callback handler function
	string dataBits;

	cerr << "integertransform: addTruncationCheck(): stub: " << p_annotation.toString() << endl;

	if (p_annotation.isUnknownSign() || p_annotation.isSigned())
	{
	// TESTING: handle only eax/ax for now

		// potentially sign-extended
		// need to check upper 16 bits are either all 1's or all 0's
		// (false positive posible if it was in fact unsigned)

		// 80486d2      5 INSTR CHECK TRUNCATION UNKNOWNSIGN 32 EAX 16 AX ZZ mov     [esp+2Ah], ax
		// ; make sure to use instructions that don't affect the condition flags
		//     push ecx                     ; temporary register
		//     push eax                     ; copy value of eax (since ax is used in instruction)
		//     movzx ecx, word [esp + 2]    ; copy upper 16 bits into ecx (zero-extend)
		//     jecxz continue               ; all 0's, all good
		//     not ecx                      ; flip all bits (all 1's bcomes all 0's)
		//     jecxz continue               ; originally all 1's, all good
		//
		//     <invoke handler here>
		//     nop
		//
		// continue: 
		//     pop eax                      ; restore eax
		//     pop ecx                      ; restore ecx
		//     mov [esp+2Ah], ax            ; instrumented instruction

		db_id_t fileID = p_instruction->GetAddress()->GetFileID();
		Function_t* func = p_instruction->GetFunction();

		Instruction_t* push_ecx_i = allocateNewInstruction(fileID, func);
		Instruction_t* push_eax_i = allocateNewInstruction(fileID, func);
		Instruction_t* movzx_i = allocateNewInstruction(fileID, func);
		Instruction_t* jecxz_i = allocateNewInstruction(fileID, func);
		Instruction_t* not_ecx_i = allocateNewInstruction(fileID, func);
		Instruction_t* jecxz2_i = allocateNewInstruction(fileID, func);
		Instruction_t* nop_i = allocateNewInstruction(fileID, func);
		Instruction_t* pop_ecx_i = allocateNewInstruction(fileID, func);
		Instruction_t* pop_eax_i = allocateNewInstruction(fileID, func);

		// start instrumentation
		addPushRegister(push_ecx_i, Register::ECX, push_eax_i);
		cerr << "push_ecx: " << push_ecx_i->getDisassembly() << endl;

		// insert start of instrumentation before the instrument instruction
		Instruction_t* instrumentedInstr = carefullyInsertBefore(p_instruction, push_ecx_i);
		push_ecx_i->SetFallthrough(push_eax_i);

		addPushRegister(push_eax_i, Register::EAX, movzx_i);
		cerr << "push_eax: " << push_eax_i->getDisassembly() << endl;

		// movzx ecx, word [esp + 2]    ; copy upper 16 bits into ecx (zero-extend)
		dataBits.resize(5);
		dataBits[0] = 0x0f;
		dataBits[1] = 0xb7;
		dataBits[2] = 0x4c; 
		dataBits[3] = 0x24;
		dataBits[4] = 0x02;
		addInstruction(movzx_i, dataBits, jecxz_i, NULL);
		cerr << "movzx ecx, word [esp + 2]: " << movzx_i->getDisassembly() << endl;

		//     jecxz continue               ; all 0's, all good
		dataBits.resize(2);
		dataBits[0] = 0xe3; 
		dataBits[1] = 0x00; // value doesn't matter here -- will be filled in later
		addInstruction(jecxz_i, dataBits, not_ecx_i, pop_eax_i);
		cerr << "jecxz: " << jecxz_i->getDisassembly() << endl;

		//     not ecx                      ; flip all bits (all 1's bcomes all 0's)
		dataBits.resize(2);
		dataBits[0] = 0xf7; 
		dataBits[1] = 0xd1; 
		addInstruction(not_ecx_i, dataBits, jecxz2_i, NULL);
		cerr << "not ecx: " << not_ecx_i->getDisassembly() << endl;

		//     jecxz continue               ; all 0's, all good
		dataBits.resize(2);
		dataBits[0] = 0xe3; 
		dataBits[1] = 0x00; // value doesn't matter here -- will be filled in later
		addInstruction(jecxz2_i, dataBits, nop_i, pop_eax_i);
		cerr << "jecxz2: " << jecxz2_i->getDisassembly() << endl;

		//     nop
		addNop(nop_i, pop_eax_i);
		addCallbackHandler(string(TRUNCATION_DETECTOR), instrumentedInstr, nop_i, pop_eax_i);
		cerr << "nop: " << nop_i->getDisassembly() << endl;

		//     pop eax                      ; restore eax
		addPopRegister(pop_eax_i, Register::EAX, pop_ecx_i);
		cerr << "pop eax: " << pop_eax_i->getDisassembly() << endl;

		//     pop ecx                      ; restore ecx
		addPopRegister(pop_ecx_i, Register::ECX, instrumentedInstr);
		cerr << "pop ecx: " << pop_ecx_i->getDisassembly() << endl;

	}
	else if (p_annotation.isUnsigned())
	{
		// need to check upper 16 bits are all 0's
	}
	else
	{
		// error
	}
}

// 8048565      6 INSTR CHECK OVERFLOW SIGNED 16  [ESP]+38 ZZ add     word ptr [esp+26h], 1
// 804856b      6 INSTR CHECK OVERFLOW UNSIGNED 16  [ESP]+36 ZZ add     word ptr [esp+24h], 1
//
// 80483bb      4 INSTR CHECK OVERFLOW UNKNOWNSIGN 16  AX ZZ add     ax, 7FBCh
// 80483d5      3 INSTR CHECK UNDERFLOW SIGNED 16  CX ZZ sub     cx, ax
//
// for registers:
//          <save flags>
//          TEST <r16>, <r16>
//          jns L1
//          invoke overflow16_handler
//          L1:     <restore flags>
//


void IntegerTransform::addAddSubSignedOverflowCheck16(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation)
{
	// sanity checks
    assert(getVariantIR() && p_instruction);
	assert (p_annotation.isValid() && Register::is16bit(p_annotation.getRegister()));

	string detector = string(ADDSUB_OVERFLOW_DETECTOR_SIGNED_16);

    db_id_t fileID = p_instruction->GetAddress()->GetFileID();
    Function_t* func = p_instruction->GetFunction();

	Instruction_t* pushf_i = allocateNewInstruction(fileID, func);
	Instruction_t* test_i = allocateNewInstruction(fileID, func);
	Instruction_t* jns_i = allocateNewInstruction(fileID, func);
	Instruction_t* nop_i = allocateNewInstruction(fileID, func);
	Instruction_t* popf_i = allocateNewInstruction(fileID, func);

	addPushf(pushf_i, test_i);
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, pushf_i);
	pushf_i->SetFallthrough(test_i); // do I need this here again b/c carefullyInsertBefore breaks the link?
	addTestRegister(test_i, p_annotation.getRegister(), jns_i);
	addJns(jns_i, nop_i, popf_i);
	addNop(nop_i, popf_i);
	addCallbackHandler(detector, originalInstrumentInstr, nop_i, popf_i);
	addPopf(popf_i, originalInstrumentInstr);
}

//
//      <instruction to instrument>
//      jno <originalFallthroughInstruction> 
//      pusha
//      pushf
//      push_arg <address original instruction>
//      push L1
//      ... setup detector ...
//  L1: pop_arg
//      popf
//      popa
//
// 20111024 Current policy:
//            imul, mul -- always check using jno
//            add, sub  -- use signedness information annotation to emit either jno, jnc
//
void IntegerTransform::addOverflowCheck32(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation)
{
cerr << "void IntegerTransform::addOverflowCheck32(): enter: " << p_instruction->GetComment() << endl;
	assert(getVariantIR() && p_instruction);
	
	string detector(INTEGER_OVERFLOW_DETECTOR);
	string dataBits;

	AddressID_t *jncond_a =new AddressID_t;
	/*
	AddressID_t *pusha_a =new AddressID_t;
	AddressID_t *pushf_a =new AddressID_t;
	AddressID_t *pusharg_a =new AddressID_t;
	AddressID_t *pushret_a =new AddressID_t;
	AddressID_t *poparg_a =new AddressID_t;
	AddressID_t *popf_a =new AddressID_t;
	AddressID_t *popa_a =new AddressID_t;
	*/

	jncond_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	/*
	pusha_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	pushf_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	pusharg_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	pushret_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	poparg_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	popf_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	popa_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	*/

	Instruction_t* jncond_i = new Instruction_t;
	/*
	Instruction_t* pusha_i = new Instruction_t;
	Instruction_t* pushf_i = new Instruction_t;
	Instruction_t* pusharg_i = new Instruction_t;
	Instruction_t* pushret_i = new Instruction_t;
	Instruction_t* poparg_i = new Instruction_t;
	Instruction_t* popf_i = new Instruction_t;
	Instruction_t* popa_i = new Instruction_t;
	*/

	Function_t* origFunction = p_instruction->GetFunction();

	jncond_i->SetFunction(origFunction);
	/*
	pusha_i->SetFunction(origFunction);
	pushf_i->SetFunction(origFunction);
	pusharg_i->SetFunction(origFunction);
	pushret_i->SetFunction(origFunction);
	poparg_i->SetFunction(origFunction);
	popf_i->SetFunction(origFunction);
	popa_i->SetFunction(origFunction);
	*/

	// pin the poparg instruction 
	/*
	virtual_offset_t postDetectorReturn = getAvailableAddress();
	poparg_a->SetVirtualOffset(postDetectorReturn);
	*/

	jncond_i->SetAddress(jncond_a);
	/*
	pusha_i->SetAddress(pusha_a);
	pushf_i->SetAddress(pushf_a);
	pusharg_i->SetAddress(pusharg_a);
	pushret_i->SetAddress(pushret_a);
	poparg_i->SetAddress(poparg_a);
	popf_i->SetAddress(popf_a);
	popa_i->SetAddress(popa_a);
	*/

	// set fallthrough for the original instruction
	Instruction_t* nextOrig_i = p_instruction->GetFallthrough();
	p_instruction->SetFallthrough(jncond_i); 

	// jncond 
	dataBits.resize(2);
	if (isMultiplyInstruction(p_instruction))
	{
		dataBits[0] = 0x71; // jno
		dataBits[1] = 0x00; // value doesn't matter, we will fill it in later
		detector = string(MUL_OVERFLOW_DETECTOR_32);
	cerr << "integertransform: MUL OVERFLOW 32" << endl;
	}
	else if (p_annotation.isSigned())
	{
		dataBits[0] = 0x71; // jno
		dataBits[1] = 0x00; // value doesn't matter, we will fill it in later

		if (p_annotation.getBitWidth() == 32)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_SIGNED_32);
		else if (p_annotation.getBitWidth() == 16)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_SIGNED_16);
	cerr << "integertransform: ADD/SUB OVERFLOW SIGNED 32" << endl;
	}
	else if (p_annotation.isUnsigned())
	{
		dataBits[0] = 0x73; // jnc
		dataBits[1] = 0x00; // value doesn't matter, we will fill it in later

		if (p_annotation.getBitWidth() == 32)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_UNSIGNED_32);
		else if (p_annotation.getBitWidth() == 16)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_UNSIGNED_16);
	cerr << "integertransform: ADD/SUB OVERFLOW UNSIGNED 32" << endl;
	}
	else
	{
		dataBits[0] = 0x71; // jno
		dataBits[1] = 0x00; // value doesn't matter, we will fill it in later

		if (p_annotation.getBitWidth() == 32)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_SIGNED_32);
		else if (p_annotation.getBitWidth() == 16)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_SIGNED_16);
	cerr << "integertransform: ADD/SUB OVERFLOW UNKONWN 32: assume signed for now" << endl;
	}

	jncond_i->SetDataBits(dataBits);
	jncond_i->SetComment(jncond_i->getDisassembly());
	jncond_i->SetTarget(nextOrig_i); 
//	jncond_i->SetFallthrough(pusha_i);  // this is set in addCallBackhandler

	p_instruction->SetFallthrough(jncond_i); 

	addCallbackHandler(detector, p_instruction, jncond_i, nextOrig_i);

	getVariantIR()->GetAddresses().insert(jncond_a);
	getVariantIR()->GetInstructions().insert(jncond_i);

#ifdef OLD
	// pusha   
	dataBits.resize(1);
	dataBits[0] = 0x60;
	pusha_i->SetDataBits(dataBits);
	pusha_i->SetComment(pusha_i->getDisassembly());
	pusha_i->SetFallthrough(pushf_i); 

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
	poparg_i->SetComment(poparg_i->getDisassembly() + " -- with callback to " + detector + " orig: " + p_instruction->GetComment()) ;
	poparg_i->SetFallthrough(popf_i); 
	poparg_i->SetIndirectBranchTargetAddress(poparg_a);  
	poparg_i->SetCallback(detector); 

	// popf   
	dataBits.resize(1);
	dataBits[0] = 0x9d;
	popf_i->SetDataBits(dataBits);
	popf_i->SetComment(popf_i->getDisassembly());
	popf_i->SetFallthrough(popa_i); 

	// popa   
	dataBits.resize(1);
	dataBits[0] = 0x61;
	popa_i->SetDataBits(dataBits);
	popa_i->SetComment(popa_i->getDisassembly());
	popa_i->SetFallthrough(nextOrig_i); 

	// add new address to IR
	getVariantIR()->GetAddresses().insert(jncond_a);
	getVariantIR()->GetAddresses().insert(pusha_a);
	getVariantIR()->GetAddresses().insert(pusharg_a);
	getVariantIR()->GetAddresses().insert(pushf_a);
	getVariantIR()->GetAddresses().insert(pushret_a);
	getVariantIR()->GetAddresses().insert(popf_a);
	getVariantIR()->GetAddresses().insert(poparg_a);
	getVariantIR()->GetAddresses().insert(popa_a);

	// add new instructions to IR
	getVariantIR()->GetInstructions().insert(jncond_i);
	getVariantIR()->GetInstructions().insert(pusha_i);
	getVariantIR()->GetInstructions().insert(pusharg_i);
	getVariantIR()->GetInstructions().insert(pushf_i);
	getVariantIR()->GetInstructions().insert(pushret_i);
	getVariantIR()->GetInstructions().insert(popf_i);
	getVariantIR()->GetInstructions().insert(poparg_i);
	getVariantIR()->GetInstructions().insert(popa_i);
#endif
cerr << "void IntegerTransform::addOverflowCheck32(): exit" << endl;
}

