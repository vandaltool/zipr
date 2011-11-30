#include <assert.h>
#include "integertransform.hpp"
#include "leapattern.hpp"

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
				
				if (annotation.isOverflow())
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

	addCallbackHandler(detector, originalInstrumentInstr, nop_i, popf_i, p_instruction);
	addPopf(popf_i, originalInstrumentInstr);
}

void IntegerTransform::handleOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation)
{
	if (p_annotation.isOverflow() && p_annotation.isNoFlag())
	{
		addOverflowCheckNoFlag(p_instruction, p_annotation);
	}
	else 
	if (isMultiplyInstruction(p_instruction) || p_annotation.isUnderflow() || p_annotation.isOverflow())
	{
		addOverflowCheck(p_instruction, p_annotation);
	}
	else
		cerr << "integertransform: OVERFLOW/UNDERFLOW type not yet handled" << p_annotation.toString() << endl;
}

/*
   NOFLAGUNKNOWNSIGN 32 ESI+EDX ZZ lea     eax, [esi+edx] 
   NOFLAGUNSIGNED 32 EDX+ESI+1900 ZZ lea     esi, [edx+esi+76Ch] 
   NOFLAGUNSIGNED 32 EAX+382 ZZ lea     esi, [eax+17Eh] 
   NOFLAGUNKNOWNSIGN 32 EDX*8 ZZ lea     eax, ds:0[edx*8] 
   NOFLAGUNSIGNED 32 ECX+ECX*4 ZZ lea     ebx, [ecx+ecx*4] 
   NOFLAGUNSIGNED 32 EDI+-66 ZZ lea     eax, [edi-42h] 
   NOFLAGSIGNED 32 ESI+100 ZZ lea     ecx, [esi+64h] 

   posbible patterns after the bit width field:

     rpr:  <reg>+<reg>
	 rpc:  <reg>+-<constant>
	 rpc:  <reg>+<constant>
     rtc:  <reg>*constant
     rprpc:  <reg>+<reg>+<constant>
     rprpc:  <reg>+<reg>+-<constant>
     rprtc:  <reg>+<reg>*<constant>
*/
void IntegerTransform::addOverflowCheckNoFlag(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation)
{

		cerr << "IntegerTransform::addOverflowCheckNoFlag(): Disable for now" << endl;
		return;

	LEAPattern leaPattern(p_annotation);

	if (!leaPattern.isValid())
	{
		cerr << "IntegerTransform::addOverflowCheckNoFlag(): invalid lea pattern - skipping: " << p_annotation.toString() << endl;
		return;
	}

	if (leaPattern.isRegisterPlusRegister())
	{
		Register::RegisterName reg1 = leaPattern.getRegister1();
		Register::RegisterName reg2 = leaPattern.getRegister2();
		Register::RegisterName target = getTargetRegister(p_instruction);

		if (reg1 == Register::UNKNOWN || reg2 == Register::UNKNOWN || target == Register::UNKNOWN)
		{
			cerr << "IntegerTransform::addOverflowCheckNoFlag(): lea reg+reg pattern: error retrieving register:" << "reg1: " << Register::toString(reg1) << " reg2: " << Register::toString(reg2) << " target: " << Register::toString(target) << endl;
			return;
		}
		else
		{
			addOverflowCheckNoFlag_RegPlusReg(p_instruction, p_annotation, reg1, reg2, target);
		}
	}
	else if (leaPattern.isRegisterPlusConstant())
	{
	return;
		Register::RegisterName reg1 = leaPattern.getRegister1();
		int value = leaPattern.getConstant();
		Register::RegisterName target = getTargetRegister(p_instruction);

		if (reg1 == Register::UNKNOWN || target == Register::UNKNOWN)
		{
			cerr << "IntegerTransform::addOverflowCheckNoFlag(): lea reg+constant pattern: error retrieving register:" << "reg1: " << Register::toString(reg1) << " target: " << Register::toString(target) << endl;
			return;
		}
		else
		{
			addOverflowCheckNoFlag_RegPlusConstant(p_instruction, p_annotation, reg1, value, target);
		}
	}
	else if (leaPattern.isRegisterTimesConstant())
	{
	return;
		Register::RegisterName reg1 = leaPattern.getRegister1();
		int value = leaPattern.getConstant();
		Register::RegisterName target = getTargetRegister(p_instruction);

		if (reg1 == Register::UNKNOWN || target == Register::UNKNOWN)
		{
			cerr << "IntegerTransform::addOverflowCheckNoFlag(): lea reg*constant pattern: error retrieving register:" << "reg1: " << Register::toString(reg1) << " target: " << Register::toString(target) << endl;
			return;
		}
		else
		{
			addOverflowCheckNoFlag_RegTimesConstant(p_instruction, p_annotation, reg1, value, target);
		}
	}
	else
	{
		cerr << "IntegerTransform::addOverflowCheckNoFlag(): pattern not yet handled: " << p_annotation.toString() << endl;
		return;
	}

}

// Example annotation to handle
// 804852e      3 INSTR CHECK OVERFLOW NOFLAGSIGNED 32 EDX+EAX ZZ lea     eax, [edx+eax] Reg1: EDX Reg2: EAX
void IntegerTransform::addOverflowCheckNoFlag_RegPlusReg(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const Register::RegisterName& p_reg2, const Register::RegisterName& p_reg3)
{
// should we even attempt to instrument if we're not sure about the signedness for this pattern?

// need to use the correct PC (the one for the original lea instruction) when printing detector message

	// Instrumentation:
	//   pushf                  ;     save flags
	//   add r1, r2             ;     r1 = r1 + r2;
	//   <check for overflow>   ;     reuse overflow code
	//   mov r3, r1             ;     r3 = r1  (replaces lea r3, [r1+r2])
	//                          ;     possible optimization: do nothing if r3 == r1
	//   popf                   ;     restore flags
	//

	db_id_t fileID = p_instruction->GetAddress()->GetFileID();
	Function_t* func = p_instruction->GetFunction();

	Instruction_t* fallthrough = p_instruction->GetFallthrough();
	Instruction_t* pushf_i = allocateNewInstruction(fileID, func);
	Instruction_t* addR1R2_i = allocateNewInstruction(fileID, func);
	Instruction_t* movR3R1_i = allocateNewInstruction(fileID, func);
	Instruction_t* popf_i = allocateNewInstruction(fileID, func);

	MEDS_InstructionCheckAnnotation addR1R2_annot;
	addR1R2_annot.setValid();
	addR1R2_annot.setBitWidth(32);
	addR1R2_annot.setOverflow();
	if (p_annotation.isSigned())
		addR1R2_annot.setSigned();
	else if (p_annotation.isUnsigned())
		addR1R2_annot.setUnsigned();
	else
		addR1R2_annot.setUnknownSign();

	string msg = "Originally: " + p_instruction->getDisassembly();

	addPushf(pushf_i, addR1R2_i);
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, pushf_i);
	pushf_i->SetFallthrough(addR1R2_i); // do I need this?

	addAddRegisters(addR1R2_i, p_reg1, p_reg2, movR3R1_i);
	addMovRegisters(movR3R1_i, p_reg3, p_reg1, popf_i);
	addPopf(popf_i, fallthrough);

	movR3R1_i->SetComment(msg);
	addOverflowCheck(addR1R2_i, addR1R2_annot);
}

void IntegerTransform::addOverflowCheckNoFlag_RegPlusConstant(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const int p_constantValue, const Register::RegisterName& p_reg3)
{
	cerr << "integertransform: reg+constant: register: " << Register::toString(p_reg1) << " constant: " << p_constantValue << " target register: " << Register::toString(p_reg3) << "  annotation: " << p_annotation.toString() << endl;
// should we even attempt to instrument if we're not sure about the signedness for this pattern?

// need to use the correct PC (the one for the original lea instruction) when printing detector message

	// Original instruction is of the form:            lea r3, [r1+constant]          
	// Instrumentation:
	//   pushf                  ;     save flags
	//   add r1, constant       ;     r1 = r1 + constant;
	//   <check for overflow>   ;     reuse overflow code
	//   mov r3, r1             ;     r3 = r1  (replaces lea r3, [r1+constant])
	//                          ;     possible optimization: do nothing if r3 == r1
	//   popf                   ;     restore flags
	//

	db_id_t fileID = p_instruction->GetAddress()->GetFileID();
	Function_t* func = p_instruction->GetFunction();

	Instruction_t* fallthrough = p_instruction->GetFallthrough();
	Instruction_t* pushf_i = allocateNewInstruction(fileID, func);
	Instruction_t* addR1Constant_i = allocateNewInstruction(fileID, func);
	Instruction_t* movR3R1_i = allocateNewInstruction(fileID, func);
	Instruction_t* popf_i = allocateNewInstruction(fileID, func);

	MEDS_InstructionCheckAnnotation addR1Constant_annot;
	addR1Constant_annot.setValid();
	addR1Constant_annot.setBitWidth(32);
	addR1Constant_annot.setOverflow();
	if (p_annotation.isSigned())
		addR1Constant_annot.setSigned();
	else if (p_annotation.isUnsigned())
		addR1Constant_annot.setUnsigned();
	else
		addR1Constant_annot.setUnknownSign();

	string msg = "Originally: " + p_instruction->getDisassembly();

	addPushf(pushf_i, addR1Constant_i);
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, pushf_i);
	pushf_i->SetFallthrough(addR1Constant_i); // do I need this?

	addAddRegisterConstant(addR1Constant_i, p_reg1, p_constantValue, movR3R1_i);
	addMovRegisters(movR3R1_i, p_reg3, p_reg1, popf_i);
	addPopf(popf_i, fallthrough);

	movR3R1_i->SetComment(msg);
	addOverflowCheck(addR1Constant_i, addR1Constant_annot);
}

void IntegerTransform::addOverflowCheckNoFlag_RegTimesConstant(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const int p_constantValue, const Register::RegisterName& p_reg3)
{
	cerr << "integertransform: reg+constant: register: " << Register::toString(p_reg1) << " constant: " << p_constantValue << " target register: " << Register::toString(p_reg3) << "  annotation: " << p_annotation.toString() << endl;
// should we even attempt to instrument if we're not sure about the signedness for this pattern?

// need to use the correct PC (the one for the original lea instruction) when printing detector message

	// Original instruction is of the form:            lea r3, [r1+constant]          
	// Instrumentation:
	//   pushf                  ;     save flags
	//   add r1, constant       ;     r1 = r1 + constant;
	//   <check for overflow>   ;     reuse overflow code
	//   mov r3, r1             ;     r3 = r1  (replaces lea r3, [r1+constant])
	//                          ;     possible optimization: do nothing if r3 == r1
	//   popf                   ;     restore flags
	//

	db_id_t fileID = p_instruction->GetAddress()->GetFileID();
	Function_t* func = p_instruction->GetFunction();

	Instruction_t* fallthrough = p_instruction->GetFallthrough();
	Instruction_t* pushf_i = allocateNewInstruction(fileID, func);
	Instruction_t* mulR1Constant_i = allocateNewInstruction(fileID, func);
	Instruction_t* movR3R1_i = allocateNewInstruction(fileID, func);
	Instruction_t* popf_i = allocateNewInstruction(fileID, func);

	MEDS_InstructionCheckAnnotation mulR1Constant_annot;
	mulR1Constant_annot.setValid();
	mulR1Constant_annot.setBitWidth(32);
	mulR1Constant_annot.setOverflow();
	if (p_annotation.isSigned())
		mulR1Constant_annot.setSigned();
	else if (p_annotation.isUnsigned())
		mulR1Constant_annot.setUnsigned();
	else
		mulR1Constant_annot.setUnknownSign();

	string msg = "Originally: " + p_instruction->getDisassembly();

	addPushf(pushf_i, mulR1Constant_i);
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, pushf_i);
	pushf_i->SetFallthrough(mulR1Constant_i); // do I need this?

	addMulRegisterConstant(mulR1Constant_i, p_reg1, p_constantValue, movR3R1_i);
	addMovRegisters(movR3R1_i, p_reg3, p_reg1, popf_i);
	addPopf(popf_i, fallthrough);

	movR3R1_i->SetComment(msg);
	addOverflowCheck(mulR1Constant_i, mulR1Constant_annot);
	return;
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
cerr << "IntegerTransform::addOverflowCheck(): instr: " << p_instruction->getDisassembly() << " address: " << p_instruction->GetAddress() << " annotation: " << p_annotation.toString() << endl;
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
	addCallbackHandler(detector, originalInstrumentInstr, nop_i, popf_i, p_instruction);
	addPopf(popf_i, originalInstrumentInstr);
}
