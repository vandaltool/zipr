#include <assert.h>
#include "integertransform.hpp"
#include "leapattern.hpp"

//
// MEDS has the following annotation types:
//     - OVERFLOW (SIGNED|UNSIGNED|UNKNOWN) (32,16,8)
//     - UNDERFLOW (SIGNED|UNSIGNED|UNKNOWN) (32,16,8)
//     - SIGNEDNESS SIGNED (32,16,8)
//     - TRUNCATION (SIGNED|UNSIGNED|UNKNOWN) (32,16,8)
//     - XXX_NOFLAG (Many forms, handles LEA)
//
// Saturating arithmetic implemented for:
//      - OVERFLOW (when destination is a register)
//      - UNDERFLOW (when destination is a register)
//      - SIGNEDNESS 
//      - TRUNCATION
//
// ============= TO DO ============= 
// Saturating arithmetic to do:
//      - OVERFLOW (dest. is not a register)
//      - UNDERFLOW (dest. is not a register)
//
// Instrumentation:
//      - TRUNCATION (16->8)   no test cases available
//      - LEA                  only reg32+reg32 case implemented
//
using namespace libTransform;

IntegerTransform::IntegerTransform(VariantID_t *p_variantID, VariantIR_t *p_variantIR, std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *p_annotations, set<std::string> *p_filteredFunctions, set<VirtualOffset> *p_benignFalsePositives) : Transform(p_variantID, p_variantIR, p_annotations, p_filteredFunctions) 
{
	m_benignFalsePositives = p_benignFalsePositives;
}

// iterate through all functions
// filter those functions that should be ignored
//    iterate through all instructions in function
//    if MEDS annotation says to instrument
//       add instrumentation
int IntegerTransform::execute()
{
	if (getSaturatingArithmetic())
		cerr << "IntegerTransform: Saturating Arithmetic is enabled" << endl;
	else
		cerr << "IntegerTransform: Saturating Arithmetic is disabled" << endl;

	for(
	  set<Function_t*>::const_iterator itf=getVariantIR()->GetFunctions().begin();
	  itf!=getVariantIR()->GetFunctions().end();
	  ++itf
	  )
	{
		Function_t* func=*itf;

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
				int policy = POLICY_DEFAULT; // use Strata default settings
				virtual_offset_t irdb_vo = insn->GetAddress()->GetVirtualOffset();
				if (irdb_vo == 0) continue;

				VirtualOffset vo(irdb_vo);

				if (m_benignFalsePositives && m_benignFalsePositives->count(vo))
				{
					// potential benign false positives
					policy = POLICY_CONTINUE;
				}
				else if (getSaturatingArithmetic())
				{
					// saturating arithmetic is enabled
					// only use if instruction is not a potential false positive
					policy = POLICY_CONTINUE_SATURATING_ARITHMETIC;
				}

				MEDS_InstructionCheckAnnotation annotation = (*getAnnotations())[vo];
				if (!annotation.isValid()) 
					continue;
				
				if (annotation.isOverflow())
				{
					// nb: safe with respect to esp (except for lea)
					handleOverflowCheck(insn, annotation, policy);
				}
				else if (annotation.isUnderflow() && !annotation.isNoFlag())
				{
					// nb: safe with respect to esp
					handleUnderflowCheck(insn, annotation, policy);
				}
				else if (annotation.isTruncation())
				{
					// nb: safe with respect to esp
					handleTruncation(insn, annotation, policy);

				}
				else if (annotation.isSignedness())
				{
					handleSignedness(insn, annotation, policy);
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

void IntegerTransform::handleSignedness(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	if (p_annotation.isSigned() || p_annotation.isUnsigned())
		addSignednessCheck(p_instruction, p_annotation, p_policy);
	else
		cerr << "handleSignedness(): case not yet handled" << endl;
}

// 8048576 5 INSTR CHECK SIGNEDNESS SIGNED 16 AX ZZ mov     [esp+28h], ax
//             <save flags>
//             TEST ax, ax
//             jns L1
//             invoke conversion handler
// (optional)  mov ax, MAX_16     ; saturating arithmetic (Max for bit width/sign/unsigned) 
//  
//       L1:   <restore flags>
//             mov [esp+28h], ax
//
void IntegerTransform::addSignednessCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
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
      cerr << "addSignednessCheck(): Unexpected bit width and register combination: skipping instrumentation for: " << p_annotation.toString() << endl;
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

	// sanity filter
	if (p_annotation.getRegister() == Register::UNKNOWN)
		p_policy = POLICY_DEFAULT;

	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
	{
		// implement saturating arithmetic on register, i.e.: mov <reg>, value
		Instruction_t* saturate_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());

		addCallbackHandler(detector, originalInstrumentInstr, nop_i, saturate_i, p_policy, p_instruction->GetAddress());
		if (p_annotation.isSigned())
			addMaxSaturation(saturate_i, p_annotation.getRegister(), p_annotation, popf_i);
		else
			addZeroSaturation(saturate_i, p_annotation.getRegister(), popf_i);
	}
	else
	{
		addCallbackHandler(detector, originalInstrumentInstr, nop_i, popf_i, p_policy, p_instruction->GetAddress());
	}
	addPopf(popf_i, originalInstrumentInstr);
}

void IntegerTransform::handleOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	if (p_annotation.isOverflow() && p_annotation.isNoFlag())
	{
		addOverflowCheckNoFlag(p_instruction, p_annotation, p_policy);
	}
	else if (isMultiplyInstruction(p_instruction) || p_annotation.isUnderflow() || p_annotation.isOverflow())
	{
		addOverflowCheck(p_instruction, p_annotation, p_policy);
	}
	else
		cerr << "integertransform: OVERFLOW type not yet handled" << p_annotation.toString() << endl;
}

void IntegerTransform::handleUnderflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	if (p_annotation.isUnderflow())
	{
		addUnderflowCheck(p_instruction, p_annotation, p_policy);
	}
	else
		cerr << "integertransform: UNDERFLOW type not yet handled" << p_annotation.toString() << endl;
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
void IntegerTransform::addOverflowCheckNoFlag(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	LEAPattern leaPattern(p_annotation);

	if (!leaPattern.isValid())
	{
		cerr << "IntegerTransform::addOverflowCheckNoFlag(): invalid or unhandled lea pattern - skipping: " << p_annotation.toString() << endl;
		return;
	}
	
	if (p_annotation.isUnknownSign())
	{
		cerr << "IntegerTransform::addOverflowCheckNoFlag(): unknown sign -- skipping: " << p_annotation.toString() << endl;
		return;
	}

	if (leaPattern.getRegister1() == Register::UNKNOWN ||
		leaPattern.getRegister1() == Register::ESP)
	{
		cerr << "IntegerTransform::addOverflowCheckNoFlag(): destination register is unknown, esp or ebp -- skipping: " << p_annotation.toString() << endl;
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
		else if (reg2 == Register::ESP)
		{
			cerr << "IntegerTransform::addOverflowCheckNoFlag(): source register is esp or ebp -- skipping: " << p_annotation.toString() << endl;
			return;
		}
		else
		{
			addOverflowCheckNoFlag_RegPlusReg(p_instruction, p_annotation, reg1, reg2, target, p_policy);
		}
	}
	else if (leaPattern.isRegisterPlusConstant())
	{
		Register::RegisterName reg1 = leaPattern.getRegister1();
		int value = leaPattern.getConstant();
		Register::RegisterName target = getTargetRegister(p_instruction);

		if (p_annotation.isUnsigned() && value < 0)
		{
			cerr << "IntegerTransform::addOverflowCheckNoFlag(): lea reg+neg constant pattern: skip this annotation type (prone to false positives)" << endl;
			return;
		}
		else if (reg1 == Register::UNKNOWN || target == Register::UNKNOWN)
		{
			cerr << "IntegerTransform::addOverflowCheckNoFlag(): lea reg+constant pattern: error retrieving register:" << "reg1: " << Register::toString(reg1) << " target: " << Register::toString(target) << endl;
			return;
		}
		else
		{
			addOverflowCheckNoFlag_RegPlusConstant(p_instruction, p_annotation, reg1, value, target, p_policy);
		}
	}
	else if (leaPattern.isRegisterTimesConstant())
	{
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
			addOverflowCheckNoFlag_RegTimesConstant(p_instruction, p_annotation, reg1, value, target, p_policy);
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
void IntegerTransform::addOverflowCheckNoFlag_RegPlusReg(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const Register::RegisterName& p_reg2, const Register::RegisterName& p_reg3, int p_policy)
{
// should we even attempt to instrument if we're not sure about the signedness for this pattern?

//	cerr << "IntegerTransform::addOverflowCheckNoFlag_RegPlusReg(): " << p_annotation.toString() << endl;
	//
	// Instrumentation for: lea r3, [r1+r2]
	//   pushf                  ;     save flags

	// (when r3 == r1)
	//   add r3, r2             ;     r3 = (r3==r1) + r2
	// (when r3 == r2)
	//   add r3, r1             ;     r3 = (r3==r2) + r1
	// (otherwise)
	//   mov r3, r1             ;     r3 = r1
	//   add r3, r2             ;     r3 = r2 + r1

	//   <check for overflow>   ;     reuse overflow code
	//   popf                   ;     restore flags
	//   <nextIntruction>

	db_id_t fileID = p_instruction->GetAddress()->GetFileID();
	Function_t* func = p_instruction->GetFunction();

	Instruction_t* fallthrough = p_instruction->GetFallthrough();
	Instruction_t* pushf_i = allocateNewInstruction(fileID, func);
	Instruction_t* movR3R1_i = NULL;
	Instruction_t* addRR_i = allocateNewInstruction(fileID, func);
	Instruction_t* popf_i = allocateNewInstruction(fileID, func);

	MEDS_InstructionCheckAnnotation addRR_annot;
	addRR_annot.setValid();
	addRR_annot.setBitWidth(32);
	addRR_annot.setOverflow();
	if (p_annotation.isSigned())
		addRR_annot.setSigned();
	else if (p_annotation.isUnsigned())
		addRR_annot.setUnsigned();
	else
		addRR_annot.setUnknownSign();

	string msg = "Originally: " + p_instruction->getDisassembly();
	AddressID_t *originalAddress = p_instruction->GetAddress();

	bool hasSameRegister = false;
	if (p_reg3 == p_reg1 || p_reg3 == p_reg2)
		hasSameRegister = true;
	else
		movR3R1_i = allocateNewInstruction(fileID, func);

	if (hasSameRegister)
		addPushf(pushf_i, addRR_i);
	else
		addPushf(pushf_i, movR3R1_i);

	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, pushf_i);
	if (hasSameRegister)
		pushf_i->SetFallthrough(addRR_i); // do I need this?
	else
		pushf_i->SetFallthrough(movR3R1_i); // do I need this?

	if (hasSameRegister)
	{
		if (p_reg3 == p_reg1)
			addAddRegisters(addRR_i, p_reg3, p_reg2, popf_i);
		else if (p_reg3 == p_reg2)
			addAddRegisters(addRR_i, p_reg3, p_reg1, popf_i);
	}
	else
	{
		addMovRegisters(movR3R1_i, p_reg3, p_reg1, addRR_i);
		addAddRegisters(addRR_i, p_reg3, p_reg2, popf_i);
	}
	addPopf(popf_i, fallthrough);

	addRR_i->SetComment(msg);
	addOverflowCheck(addRR_i, addRR_annot, p_policy, originalAddress);
}

void IntegerTransform::addOverflowCheckNoFlag_RegPlusConstant(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const int p_constantValue, const Register::RegisterName& p_reg3, int p_policy)
{
//	cerr << "integertransform: reg+constant: register: " << Register::toString(p_reg1) << " constant: " << p_constantValue << " target register: " << Register::toString(p_reg3) << "  annotation: " << p_annotation.toString() << endl;

	//
	// Original instruction is of the form:
	//   lea r3, [r1+constant]          
	//   lea r3, [r1-constant]          
	//
	// Example annotation:
	//   8049410 3 INSTR CHECK OVERFLOW NOFLAGUNSIGNED 32 EAX+-15 ZZ lea ebx, [eax-0Fh]
	//
	// In this example:
	//   r3 = ebx
	//   r1 = eax
	//   constant = -15
	//
	// Instrumentation:
	//   pushf                  ;     save flags
	//   mov r3, r1             ;     r3 = r1
	//   add r3, constant       ;     r3 = r1 + constant;
	//   <check for overflow>   ;     reuse overflow code
	//   popf                   ;     restore flags
	//
	// Note: if r3 == r1, code still works (though inefficiently)
	//
	db_id_t fileID = p_instruction->GetAddress()->GetFileID();
	Function_t* func = p_instruction->GetFunction();

	Instruction_t* fallthrough = p_instruction->GetFallthrough();
	Instruction_t* pushf_i = allocateNewInstruction(fileID, func);
	Instruction_t* movR3R1_i = allocateNewInstruction(fileID, func);
	Instruction_t* addR3Constant_i = allocateNewInstruction(fileID, func);
	Instruction_t* popf_i = allocateNewInstruction(fileID, func);

	MEDS_InstructionCheckAnnotation addR3Constant_annot;
	addR3Constant_annot.setValid();
	addR3Constant_annot.setBitWidth(32);
	addR3Constant_annot.setOverflow();
	if (p_annotation.isSigned())
		addR3Constant_annot.setSigned();
	else if (p_annotation.isUnsigned())
		addR3Constant_annot.setUnsigned();
	else
		addR3Constant_annot.setUnknownSign();

	string msg = "Originally: " + p_instruction->getDisassembly();

	AddressID_t *originalAddress = p_instruction->GetAddress();

	addPushf(pushf_i, movR3R1_i);
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, pushf_i);
	pushf_i->SetFallthrough(movR3R1_i); // do I need this?

	addMovRegisters(movR3R1_i, p_reg3, p_reg1, addR3Constant_i);
	addAddRegisterConstant(addR3Constant_i, p_reg3, p_constantValue, popf_i);
	addPopf(popf_i, fallthrough);

	addR3Constant_i->SetComment(msg);
	addOverflowCheck(addR3Constant_i, addR3Constant_annot, p_policy, originalAddress);
}

void IntegerTransform::addOverflowCheckNoFlag_RegTimesConstant(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const int p_constantValue, const Register::RegisterName& p_reg3, int p_policy)
{
//	cerr << "integertransform: reg*constant: register: " << Register::toString(p_reg1) << " constant: " << p_constantValue << " target register: " << Register::toString(p_reg3) << "  annotation: " << p_annotation.toString() << endl;
	//
	// Original instruction is of the form:
	//   lea r3, [r1*constant]          
	//
	// Instrumentation:
	//   pushf                  ;     save flags
	//   mov r3, r1             ;     r3 = r1
	//   imul r3, constant      ;     r3 = r1 * constant;
	//   <check for overflow>   ;     reuse overflow code
	//   popf                   ;     restore flags
	//
	// Note: if r3 == r1, code still works (though inefficiently)
	//
	db_id_t fileID = p_instruction->GetAddress()->GetFileID();
	Function_t* func = p_instruction->GetFunction();

	Instruction_t* fallthrough = p_instruction->GetFallthrough();
	Instruction_t* pushf_i = allocateNewInstruction(fileID, func);
	Instruction_t* movR3R1_i = allocateNewInstruction(fileID, func);
	Instruction_t* mulR3Constant_i = allocateNewInstruction(fileID, func);
	Instruction_t* popf_i = allocateNewInstruction(fileID, func);

	MEDS_InstructionCheckAnnotation mulR3Constant_annot;
	mulR3Constant_annot.setValid();
	mulR3Constant_annot.setBitWidth(32);
	mulR3Constant_annot.setOverflow();
	if (p_annotation.isSigned())
		mulR3Constant_annot.setSigned();
	else if (p_annotation.isUnsigned())
		mulR3Constant_annot.setUnsigned();
	else
		mulR3Constant_annot.setUnknownSign();

	string msg = "Originally: " + p_instruction->getDisassembly();

	AddressID_t *originalAddress = p_instruction->GetAddress();

	addPushf(pushf_i, movR3R1_i);
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, pushf_i);
	pushf_i->SetFallthrough(movR3R1_i); // do I need this?

	addMovRegisters(movR3R1_i, p_reg3, p_reg1, mulR3Constant_i);
	addMulRegisterConstant(mulR3Constant_i, p_reg3, p_constantValue, popf_i);
	addPopf(popf_i, fallthrough);

	mulR3Constant_i->SetComment(msg);
	addOverflowCheck(mulR3Constant_i, mulR3Constant_annot, p_policy, originalAddress);
}

void IntegerTransform::handleTruncation(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	if (p_annotation.getTruncationFromWidth() == 32 && (p_annotation.getTruncationToWidth() == 16 || p_annotation.getTruncationToWidth() == 8))
	{
		addTruncationCheck(p_instruction, p_annotation, p_policy);
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
void IntegerTransform::addOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy, AddressID_t *p_addressOriginalInstruction)
{
	assert(getVariantIR() && p_instruction);

//cerr << "IntegerTransform::addOverflowCheck(): instr: " << p_instruction->getDisassembly() << " address: " << p_instruction->GetAddress() << " annotation: " << p_annotation.toString() << " policy: " << p_policy << endl;
	
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
//		cerr << "integertransform: MUL OVERFLOW: " << detector << endl;
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
//		cerr << "integertransform: UNSIGNED OVERFLOW: " << detector << endl;
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
//		cerr << "integertransform: SIGNED OVERFLOW: " << detector << endl;
	}

	jncond_i->SetDataBits(dataBits);
	jncond_i->SetComment(jncond_i->getDisassembly());
	jncond_i->SetTarget(nextOrig_i); 

	p_instruction->SetFallthrough(jncond_i); 

	Register::RegisterName targetReg = getTargetRegister(p_instruction);
	if (targetReg == Register::UNKNOWN)
		p_policy = POLICY_DEFAULT;

	if (p_addressOriginalInstruction)
	{
		addCallbackHandler(detector, p_instruction, jncond_i, nextOrig_i, p_policy, p_addressOriginalInstruction);
	}
	else if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
	{
		// implement saturating arithmetic, e.g.:
		// mov <reg>, value
		Instruction_t* saturate_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());

		addCallbackHandler(detector, p_instruction, jncond_i, saturate_i, p_policy);
		addMaxSaturation(saturate_i, targetReg, p_annotation, nextOrig_i);
	}
	else
	{
		addCallbackHandler(detector, p_instruction, jncond_i, nextOrig_i, p_policy);
	}

	getVariantIR()->GetAddresses().insert(jncond_a);
	getVariantIR()->GetInstructions().insert(jncond_i);
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
//            add, sub  -- use signedness information annotation to emit either jno, jnc
//
void IntegerTransform::addUnderflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
//cerr << "IntegerTransform::addUnderflowCheck(): instr: " << p_instruction->getDisassembly() << " address: " << p_instruction->GetAddress() << " annotation: " << p_annotation.toString() << endl;

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
	if (p_annotation.isUnsigned())
	{
		dataBits[0] = 0x73; // jnc
		dataBits[1] = 0x00; // value doesn't matter, we will fill it in later

		if (p_annotation.getBitWidth() == 32)
			detector = string(UNDERFLOW_DETECTOR_UNSIGNED_32);
		else if (p_annotation.getBitWidth() == 16)
			detector = string(UNDERFLOW_DETECTOR_UNSIGNED_16);
		else if (p_annotation.getBitWidth() == 8)
			detector = string(UNDERFLOW_DETECTOR_UNSIGNED_8);
//		cerr << "integertransform: UNSIGNED OVERFLOW: " << detector << endl;
	}
	else if (p_annotation.isSigned())
	{
		dataBits[0] = 0x71; // jno
		dataBits[1] = 0x00; // value doesn't matter, we will fill it in later

		if (p_annotation.getBitWidth() == 32)
			detector = string(UNDERFLOW_DETECTOR_SIGNED_32);
		else if (p_annotation.getBitWidth() == 16)
			detector = string(UNDERFLOW_DETECTOR_SIGNED_16);
		else if (p_annotation.getBitWidth() == 8)
			detector = string(UNDERFLOW_DETECTOR_SIGNED_8);

//		cerr << "integertransform: SIGNED UNDERFLOW: " << detector << endl;
	}
	else
	{
		dataBits[0] = 0x71; // jno
		dataBits[1] = 0x00; // value doesn't matter, we will fill it in later

		if (p_annotation.getBitWidth() == 32)
			detector = string(UNDERFLOW_DETECTOR_32);
		else if (p_annotation.getBitWidth() == 16)
			detector = string(UNDERFLOW_DETECTOR_16);
		else if (p_annotation.getBitWidth() == 8)
			detector = string(UNDERFLOW_DETECTOR_8);
//		cerr << "integertransform: UNDERFLOW UNKONWN: assume signed for now: " << detector << endl;
	}

	jncond_i->SetDataBits(dataBits);
	jncond_i->SetComment(jncond_i->getDisassembly());
	jncond_i->SetTarget(nextOrig_i); 

	p_instruction->SetFallthrough(jncond_i); 

	Register::RegisterName targetReg = getTargetRegister(p_instruction);
	if (targetReg == Register::UNKNOWN)
		p_policy = POLICY_DEFAULT;

	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
	{
		// implement saturating arithmetic, e.g.:
		// mov <reg>, value
		Instruction_t* saturate_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());

		addCallbackHandler(detector, p_instruction, jncond_i, saturate_i, p_policy);
		addMinSaturation(saturate_i, targetReg, p_annotation, nextOrig_i);
	}
	else
	{
		addCallbackHandler(detector, p_instruction, jncond_i, nextOrig_i, p_policy);
	}

	getVariantIR()->GetAddresses().insert(jncond_a);
	getVariantIR()->GetInstructions().insert(jncond_i);
}

void IntegerTransform::addTruncationCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	assert(getVariantIR() && p_instruction);
	assert(p_annotation.getTruncationFromWidth() == 32 && p_annotation.getTruncationToWidth() == 8 || p_annotation.getTruncationToWidth() == 16);

//cerr << "IntegerTransform::addTruncationCheck(): instr: " << p_instruction->getDisassembly() << " address: " << p_instruction->GetAddress() << " annotation: " << p_annotation.toString() << " policy: " << p_policy << endl;
	string detector; 

	// 80484ed 3 INSTR CHECK TRUNCATION UNSIGNED 32 EAX 8 AL ZZ mov     [ebp+var_4], al
	// Unsigned: example: for signed truncation - 8 bit on AL
	//			   it's ok if 24 upper bits are all 1's or all 0's
	//
	//             <save flags>
	//             test eax, 0xFFFFFF00   ; (for 8 bit) 
	//             jz <continue>          ; upper 24 bits are 0's 
	//
	//             push eax               ; save eax
	//             not eax
	//             test eax, 0xFFFFFF00   ;(for 8 bit) 
	//             jz <L1>                ; upper 24 bits are 1's
	//             (invoke truncation handler) nop ; 
	//       L1:   pop eax
	//      SAT:   saturating-arithmetic  ; optional
	//
	// continue:   <restore flags>
	//             mov [ebp+var_4], al    ; <originalInstruction>
	//
	db_id_t fileID = p_instruction->GetAddress()->GetFileID();
	Function_t* func = p_instruction->GetFunction();
	AddressID_t *saveAddress = p_instruction->GetAddress();

	Instruction_t* pushf_i = allocateNewInstruction(fileID, func);
	Instruction_t* test_i = allocateNewInstruction(fileID, func);
	Instruction_t* jz_i = allocateNewInstruction(fileID, func);
	Instruction_t* popf_i = allocateNewInstruction(fileID, func);
	Instruction_t* saturate_i = NULL;
	
	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
		saturate_i = allocateNewInstruction(fileID, func);

	addPushf(pushf_i, test_i);
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, pushf_i);
	pushf_i->SetFallthrough(test_i); // do I need this here again b/c carefullyInsertBefore breaks the link?

	unsigned mask = 0;
	if (p_annotation.getTruncationToWidth() == 16)
	{
		mask = 0xFFFF0000;	
		if (p_annotation.isUnsigned())
			detector = string(TRUNCATION_DETECTOR_UNSIGNED_32_16);
		else if (p_annotation.isSigned())
			detector = string(TRUNCATION_DETECTOR_SIGNED_32_16);
		else
			detector = string(TRUNCATION_DETECTOR_32_16);
	}
	else if (p_annotation.getTruncationToWidth() == 8)
	{
		mask = 0xFFFFFF00;	
		if (p_annotation.isUnsigned())
			detector = string(TRUNCATION_DETECTOR_UNSIGNED_32_8);
		else if (p_annotation.isSigned())
			detector = string(TRUNCATION_DETECTOR_SIGNED_32_8);
		else
			detector = string(TRUNCATION_DETECTOR_32_8);
	}
			
	addTestRegisterMask(test_i, p_annotation.getRegister(), mask, jz_i);

	// add saturation instruction
	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
		addMaxSaturation(saturate_i, p_annotation.getRegister(), p_annotation, popf_i);

/*
	if (p_annotation.isUnsigned())
	{
		Instruction_t* nop_i = allocateNewInstruction(fileID, func);
		addJz(jz_i, nop_i, popf_i);

		if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
		{
			addNop(nop_i, saturate_i);
			nop_i->SetComment("UNSIGNED TRUNC: fallthrough: saturating arithmetic instruction");
			addCallbackHandler(detector, originalInstrumentInstr, nop_i, saturate_i, p_policy, saveAddress);
		}
		else
		{
			addNop(nop_i, popf_i);
			nop_i->SetComment(string("UNSIGNED TRUNC: fallthrough: popf"));
			addCallbackHandler(detector, originalInstrumentInstr, nop_i, popf_i, p_policy, saveAddress);
		}

	}
	else
*/

	// signed / unsigned / unknown

    //             <save flags>
    //             test eax, 0xFFFFFF00   ; (for 8 bit) 
	//
	//      for UNSIGNED:
    //             jz <continue>          ; upper 24 bits are 0's, fallthrough is nop 
	//             (HANDLER) nop
	//             <link to sat>
	//
	//      for SIGNED:
    //             jz <continue>          ; upper 24 bits are 0's, fallthrough is push eax
    //             push eax               ; save eax
    //             not eax
    //             test eax, 0xFFFFFF00   ; (for 8 bit) 
	//             jnz <detect>           ; 
	//
    //             pop eax
    // continue:   <restore flags>
    //             mov [ebp+var_4], al  ; <originalInstruction>
	//             ...
	//
	//   detect:   (HANDLER) pop eax
	//      sat:   <saturating arithmetic>
	//             <link to continue>

	Instruction_t* su_pushreg_i = allocateNewInstruction(fileID, func);
	Instruction_t* su_not_i = allocateNewInstruction(fileID, func);
	Instruction_t* su_test_i = allocateNewInstruction(fileID, func);
	Instruction_t* su_jnz_i = allocateNewInstruction(fileID, func);
	Instruction_t* su_popreg1_i = allocateNewInstruction(fileID, func);
	Instruction_t* su_popreg2_i = allocateNewInstruction(fileID, func);

	addJz(jz_i, su_pushreg_i, popf_i);
	jz_i->SetComment(string("jz - SIGNED or UNKNOWN TRUNC"));

	addPushRegister(su_pushreg_i, p_annotation.getRegister(), su_not_i);
	addNot(su_not_i, p_annotation.getRegister(), su_test_i);
	addTestRegisterMask(su_test_i, p_annotation.getRegister(), mask, su_jnz_i);

	addJnz(su_jnz_i, su_popreg1_i, su_popreg2_i); // fallthrough, target
	addPopRegister(su_popreg1_i, p_annotation.getRegister(), popf_i);

	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
	{
		addPopRegister(su_popreg2_i, p_annotation.getRegister(), saturate_i);
		addCallbackHandler(detector, originalInstrumentInstr, su_popreg2_i, saturate_i, p_policy, saveAddress);
	}
	else
	{
		addPopRegister(su_popreg2_i, p_annotation.getRegister(), popf_i);
		addCallbackHandler(detector, originalInstrumentInstr, su_popreg2_i, popf_i, p_policy, saveAddress);
	}

	addPopf(popf_i, originalInstrumentInstr);

//    cerr << "addTruncationCheck(): --- END ---" << endl;
}

void IntegerTransform::addMaxSaturation(Instruction_t *p_instruction, Register::RegisterName p_reg, const MEDS_InstructionCheckAnnotation& p_annotation, Instruction_t *p_fallthrough)
{
	assert(getVariantIR() && p_instruction);

	p_instruction->SetFallthrough(p_fallthrough);

	if (p_annotation.isUnsigned())
	{
		// use MAX_UNSIGNED for the bit width
		switch (Register::getBitWidth(p_reg))
		{
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
				cerr << "IntegerTransform::addMaxSaturation(): invalid bit width: " << p_annotation.getBitWidth() << endl;
				break;
		}
	}
	else
	{
		// treat unknown and signed the same way for overflows
		// use MAX_SIGNED for the bit width
		switch (Register::getBitWidth(p_reg))
		{
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
				cerr << "IntegerTransform::addMaxSaturation(): invalid bit width: " << p_annotation.getBitWidth() << endl;
				break;
		}
	}
}


void IntegerTransform::addMinSaturation(Instruction_t *p_instruction, Register::RegisterName p_reg, const MEDS_InstructionCheckAnnotation& p_annotation, Instruction_t *p_fallthrough)
{
	assert(getVariantIR() && p_instruction);

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
				cerr << "IntegerTransform::addMaxSaturation(): invalid bit width: " << p_annotation.getBitWidth() << endl;
				break;
		}
	}
}

void IntegerTransform::addZeroSaturation(Instruction_t *p_instruction, Register::RegisterName p_reg, Instruction_t *p_fallthrough)
{
	assert(getVariantIR() && p_instruction);

	p_instruction->SetFallthrough(p_fallthrough);

	addMovRegisterUnsignedConstant(p_instruction, p_reg, 0, p_fallthrough);
}

