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
					continue;
				
				if (annotation.isOverflow() && !annotation.isNoFlag())
				{
					handleOverflowCheck(insn, annotation);
				}
				else if (annotation.isUnderflow() && !annotation.isNoFlag())
				{
					cerr << "integertransform: underflow annotation: " << annotation.toString() << endl;
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
					cerr << "integertransform: unknown annotation: " << annotation.toString() << endl;
			}
		} // end iterate over all instructions in a function
	} // end iterate over all functions

	getVariantIR()->WriteToDB();

	// for now just be happy
	return 0;
}

void IntegerTransform::handleSignedness(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation)
{
	if (p_annotation.isSigned())
		addSignednessCheck(p_instruction, p_annotation);
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
//
// handle 8, 16, 32 bit

void IntegerTransform::addSignednessCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation)
{
	// sanity checks
    assert(getVariantIR() && p_instruction);
	assert (p_annotation.isValid());
	if (
		!(p_annotation.getBitWidth() == 32 && Register::is32bit(p_annotation.getRegister())) && 
		!(p_annotation.getBitWidth() == 16 && (Register::is16bit(p_annotation.getRegister()) ||
			p_annotation.getRegister() == Register::ESI || p_annotation.getRegister() == Register::EDI || p_annotation.getRegister() == Register::EBP)) && 
		!(p_annotation.getBitWidth() == 8 && Register::is8bit(p_annotation.getRegister())) 
		)
	{
      cerr << "addSignednessCheck(): Unexpected bit width and register combination: skipping instrumetnation for: " << p_annotation.toString() << endl;
	  return;
	}

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

	string detector;
	if (p_annotation.getBitWidth() == 32)
		detector = string(SIGNEDNESS_DETECTOR_32);
	else if (p_annotation.getBitWidth() == 16)
		detector = string(SIGNEDNESS_DETECTOR_16);
	else if (p_annotation.getBitWidth() == 8)
		detector = string(SIGNEDNESS_DETECTOR_8);

	addCallbackHandler(detector, originalInstrumentInstr, nop_i, popf_i);
	addPopf(popf_i, originalInstrumentInstr);
}

void IntegerTransform::handleOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation)
{
	if (isMultiplyInstruction(p_instruction) || p_annotation.isUnderflow() || p_annotation.isOverflow())
	{
		addOverflowCheck(p_instruction, p_annotation);
	}
	else
		cerr << "integertransform: OVERFLOW/UNDERFLOW type not yet handled" << p_annotation.toString() << endl;
}

void IntegerTransform::handleTruncation(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation)
{
	if (p_annotation.getTruncationFromWidth() == 32 && (p_annotation.getTruncationToWidth() == 16 || p_annotation.getTruncationToWidth() == 8))
	{
		addTruncationCheck(p_instruction, p_annotation);
	}
	else
	{
		cerr << "integertransform: TRUNCATION annotation not yet handled: " << p_annotation.toString() << endl;
	}
}

#ifdef NO_LONGER_USED
void IntegerTransform::addTruncationCheck32to16(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation)
{
	string detector; // name of SPRI/STRATA callback handler function
	string dataBits;

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
		//     not ecx                      ; flip all bits (all 1's becomes all 0's)
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
		cerr << "TRUNCATION: 32->16: need to handle unsigned case" << endl;
		// need to check upper 16 bits are all 0's
	}
	else
	{
		cerr << "TRUNCATION: error: unknown sign type in annotation" << endl;
		// error
	}
}
#endif

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
void IntegerTransform::addOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation)
{
	assert(getVariantIR() && p_instruction);
	
	string detector(INTEGER_OVERFLOW_DETECTOR);
	string dataBits;

	// old style of setting up these params
	// update to cleaner style
	Function_t* origFunction = p_instruction->GetFunction();
	AddressID_t *jncond_a =new AddressID_t;
	jncond_a->SetFileID(p_instruction->GetAddress()->GetFileID());
	Instruction_t* jncond_i = new Instruction_t;
	jncond_i->SetFunction(origFunction);
	jncond_i->SetAddress(jncond_a);

	// set fallthrough for the original instruction
	Instruction_t* nextOrig_i = p_instruction->GetFallthrough();
	p_instruction->SetFallthrough(jncond_i); 

	// jncond 
	dataBits.resize(2);
	if (isMultiplyInstruction(p_instruction))
	{
		dataBits[0] = 0x71; // jno
		dataBits[1] = 0x00; // value doesn't matter, we will fill it in later
		if (p_annotation.getBitWidth() == 32)
			detector = string(MUL_OVERFLOW_DETECTOR_32);
		else if (p_annotation.getBitWidth() == 16)
			detector = string(MUL_OVERFLOW_DETECTOR_16);
		else if (p_annotation.getBitWidth() == 8)
			detector = string(MUL_OVERFLOW_DETECTOR_8);
		cerr << "integertransform: MUL OVERFLOW: " << detector << endl;
	}
	else if (p_annotation.isSigned())
	{
		dataBits[0] = 0x71; // jno
		dataBits[1] = 0x00; // value doesn't matter, we will fill it in later

		if (p_annotation.getBitWidth() == 32)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_SIGNED_32);
		else if (p_annotation.getBitWidth() == 16)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_SIGNED_16);
		else if (p_annotation.getBitWidth() == 8)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_SIGNED_8);
		cerr << "integertransform: SIGNED OVERFLOW: " << detector << endl;
	}
	else if (p_annotation.isUnsigned())
	{
		dataBits[0] = 0x73; // jnc
		dataBits[1] = 0x00; // value doesn't matter, we will fill it in later

		if (p_annotation.getBitWidth() == 32)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_UNSIGNED_32);
		else if (p_annotation.getBitWidth() == 16)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_UNSIGNED_16);
		else if (p_annotation.getBitWidth() == 8)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_UNSIGNED_8);
		cerr << "integertransform: UNSIGNED OVERFLOW: " << detector << endl;
	}
	else
	{
		dataBits[0] = 0x71; // jno
		dataBits[1] = 0x00; // value doesn't matter, we will fill it in later

		if (p_annotation.getBitWidth() == 32)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_SIGNED_32);
		else if (p_annotation.getBitWidth() == 16)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_SIGNED_16);
		else if (p_annotation.getBitWidth() == 8)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_SIGNED_8);
	cerr << "integertransform: ADD/SUB OVERFLOW UNKONWN: assume signed for now: " << detector << endl;
	}

	jncond_i->SetDataBits(dataBits);
	jncond_i->SetComment(jncond_i->getDisassembly());
	jncond_i->SetTarget(nextOrig_i); 

	p_instruction->SetFallthrough(jncond_i); 

	addCallbackHandler(detector, p_instruction, jncond_i, nextOrig_i);

	getVariantIR()->GetAddresses().insert(jncond_a);
	getVariantIR()->GetInstructions().insert(jncond_i);
}

void IntegerTransform::addTruncationCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation)
{
	assert(getVariantIR() && p_instruction);
	assert(p_annotation.getTruncationFromWidth() == 32 && p_annotation.getTruncationToWidth() == 8 || p_annotation.getTruncationToWidth() == 16);

	string detector; 

	// Truncation unsigned
	// 80484ed      3 INSTR CHECK TRUNCATION UNSIGNED 32 EAX 8 AL ZZ mov     [ebp+var_4], al
	// Unsigned:  example: for signed truncation - 8 bit on AL
	//			it's ok if 24 upper bits are all 1's or all 0's
	//
	//           <save flags>
	//           test eax, 0xFFFFFF00 (for 8 bit) 
	//           jz <continue>      # upper 24 bits are 0's
	//
	//			# to support SIGNED add these 3 lines
	//           not eax
	//           test eax, 0xFFFFFF00 (for 8 bit) 
	//           jz <continue>      # upper 24 bits are 1's
	//           
	//           (invoke truncation handler) nop
	// continue:
	//           <restore flags>
	//           <originalInstruction>
	db_id_t fileID = p_instruction->GetAddress()->GetFileID();
	Function_t* func = p_instruction->GetFunction();

	Instruction_t* pushf_i = allocateNewInstruction(fileID, func);
	Instruction_t* test_i = allocateNewInstruction(fileID, func);
	Instruction_t* jz_i = allocateNewInstruction(fileID, func);
	Instruction_t* nop_i = allocateNewInstruction(fileID, func);
	Instruction_t* popf_i = allocateNewInstruction(fileID, func);

	addPushf(pushf_i, test_i);
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, pushf_i);
	pushf_i->SetFallthrough(test_i); // do I need this here again b/c carefullyInsertBefore breaks the link?

	unsigned mask = 0;
	if (p_annotation.getTruncationToWidth() == 16)
	{
		mask = 0xFFFF0000;	
		detector = string(TRUNCATION_DETECTOR_32_16);
	}
	else if (p_annotation.getTruncationToWidth() == 8)
	{
		mask = 0xFFFFFF00;	
		detector = string(TRUNCATION_DETECTOR_32_8);
	}
			
	addTestRegisterMask(test_i, p_annotation.getRegister(), mask, jz_i);
	if (p_annotation.isSigned() || p_annotation.isUnsigned())
	{
		//   not <reg>
		//   test <reg>, 0xFFFFFF00 (for 8 bit) 
		//   jz <continue>     # upper 24 bits are 1's
		Instruction_t* su_not_i = allocateNewInstruction(fileID, func);
		Instruction_t* su_test_i = allocateNewInstruction(fileID, func);
		Instruction_t* su_jz_i = allocateNewInstruction(fileID, func);

		addJz(jz_i, su_not_i, popf_i);
		jz_i->SetComment(string("jz - SIGNED or UNSIGNED TRUNC"));
		addNot(su_not_i, p_annotation.getRegister(), su_test_i);
		su_not_i->SetComment(string("NOT"));
		addTestRegisterMask(su_test_i, p_annotation.getRegister(), mask, su_jz_i);

		addJz(su_jz_i, nop_i, popf_i);
	}
	else
	{
		addJz(jz_i, nop_i, popf_i);
		jz_i->SetComment(string("jz - UNKNOWN TRUNC"));
	}

	addNop(nop_i, popf_i);
	nop_i->SetComment(string("NOP NOP"));
	addCallbackHandler(detector, originalInstrumentInstr, nop_i, popf_i);
	addPopf(popf_i, originalInstrumentInstr);
}
