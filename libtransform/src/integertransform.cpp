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
// 20130409 Anh  fixed lea reg+reg bug (we were assuming that annotation matched instruction exactly)
// 20130410 Anh  implemented shared library support -- added outer loop to iterate over all files in the driver program
// 20130411 Anh  skip instrumentation where there's no fallthrough for an instruction
//
using namespace libTransform;

IntegerTransform::IntegerTransform(VariantID_t *p_variantID, FileIR_t *p_fileIR, std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *p_annotations, set<std::string> *p_filteredFunctions, set<VirtualOffset> *p_benignFalsePositives) : Transform(p_variantID, p_fileIR, p_annotations, p_filteredFunctions) 
{
	m_benignFalsePositives = p_benignFalsePositives;
	m_policySaturatingArithmetic = false;
	m_policyWarningsOnly = false;
	m_pathManipulationDetected = false;

	m_annotations = p_annotations;              
}

// iterate through all functions
// filter those functions that should be ignored
//    iterate through all instructions in function
//    if MEDS annotation says to instrument
//       add instrumentation
int IntegerTransform::execute()
{
	if (isSaturatingArithmetic())
		cerr << "IntegerTransform: Saturating Arithmetic is enabled" << endl;
	else
		cerr << "IntegerTransform: Saturating Arithmetic is disabled" << endl;

	if (isPathManipulationDetected())
		cerr << "IntegerTransform: Exit on truncation" << endl;

	if (isWarningsOnly())
		cerr << "IntegerTransform: Warnings only mode" << endl;

	for(
	  set<Function_t*>::const_iterator itf=getFileIR()->GetFunctions().begin();
	  itf!=getFileIR()->GetFunctions().end();
	  ++itf
	  )
	{
		Function_t* func=*itf;

		if (getFilteredFunctions()->find(func->GetName()) != getFilteredFunctions()->end())
			continue;

/*
		if (isBlacklisted(func))
		{
			cerr << "Heuristic filter: " << func->GetName() << endl;
			continue;
		}
*/

		cerr << "Integer xform: processing fn: " << func->GetName() << endl;

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

				if (isSaturatingArithmetic())
				{
					// saturating arithmetic is enabled
					// only use if instruction is not a potential false positive
					policy = POLICY_CONTINUE_SATURATING_ARITHMETIC;
				}

				// takes precedence over saturation if conflict
				if (isWarningsOnly())
				{
					policy = POLICY_CONTINUE;
				}

				// overwrite the above if needed
				if (isPathManipulationDetected())
				{
					policy = POLICY_EXIT;
				}

				if (m_benignFalsePositives && m_benignFalsePositives->count(vo))
				{
					// potential benign false positives
					policy = POLICY_CONTINUE;
				}

				MEDS_InstructionCheckAnnotation annotation = (*getAnnotations())[vo];
				if (!annotation.isValid()) 
					continue;

				if (annotation.isIdiom())
					continue;

				if (!insn->GetFallthrough())
				{
					cerr << "Warning: no fall through for instruction -- skipping: " << insn->getDisassembly() << endl;
					continue;
				}

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
					handleTruncation(insn, annotation, policy);
				}
				else if (annotation.isSignedness())
				{
					if (annotation.isUnknownSign())
					{
						cerr << "integertransform: annotation has unknown sign: skipping";
						continue;
					}
					handleSignedness(insn, annotation, policy);
				}
				else if (annotation.isInfiniteLoop())
				{
					handleInfiniteLoop(insn, annotation, POLICY_EXIT);
				}
				else
					cerr << "integertransform: unknown annotation: " << annotation.toString() << endl;
			}
		} // end iterate over all instructions in a function
	} // end iterate over all functions

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
// 20120414 We may want to saturate the destination instead. This would mean putting the saturation code
//          after the instruction being instrumented
void IntegerTransform::addSignednessCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	// sanity checks
    assert(getFileIR() && p_instruction);
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

    cerr << "addSignednessCheck(): annot: " << p_annotation.toString() << endl;

    db_id_t fileID = p_instruction->GetAddress()->GetFileID();
    Function_t* func = p_instruction->GetFunction();

	Instruction_t* pushf_i = allocateNewInstruction(fileID, func);
	Instruction_t* test_i = allocateNewInstruction(fileID, func);
	Instruction_t* jns_i = allocateNewInstruction(fileID, func);
	Instruction_t* nop_i = allocateNewInstruction(fileID, func);
	Instruction_t* popf_i = allocateNewInstruction(fileID, func);

	addPushf(pushf_i, test_i);
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, pushf_i);
	pushf_i->SetFallthrough(test_i); 
	pushf_i->SetComment("-- in signedness check");
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
	if (p_annotation.isUnknownSign())
	{
		addOverflowCheckUnknownSign(p_instruction, p_annotation, p_policy);
	}
	else if (p_annotation.isOverflow() && p_annotation.isNoFlag())
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

   possible patterns after the bit width field:
     rpr:  <reg>+<reg>
	 rpc:  <reg>+-<constant>
	 rpc:  <reg>+<constant>
     rtc:  <reg>*constant


   not yet handling these:
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
		leaPattern.getRegister1() == Register::ESP ||
		leaPattern.getRegister1() == Register::EBP)
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
			cerr << "IntegerTransform::addOverflowCheckNoFlag(): source register is esp -- skipping: " << p_annotation.toString() << endl;
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
	// 20130409 Anh - new instrumentation code
	//
	//   orig:  lea r3, [r1+r2]
	//
	//   pushf                  ;   save flags
	//   push r1                ;   save r1
	//   add r1, r2             ;   r1 = r1 + r2
	//   <check for overflow>   ;   check for overflow as dictated by annotation
	//   pop r1                 ;   restore r1
	//   popf                   ;   restore flags
	//                          ;   context fully restored at this point
	//   lea r3, [r1+r2]        ;   execute original lea instruction
	//		
	// @todo:
	//     recalculate dead regs info at some point
	//     we could then optimize the code better

	db_id_t fileID = p_instruction->GetAddress()->GetFileID();
	Function_t* func = p_instruction->GetFunction();

	Instruction_t* pushf_i = allocateNewInstruction(fileID, func);
	Instruction_t* pushr1_i = allocateNewInstruction(fileID, func);
	Instruction_t* addRR_i = allocateNewInstruction(fileID, func);
	Instruction_t* popr1_i = allocateNewInstruction(fileID, func);
	Instruction_t* popf_i = allocateNewInstruction(fileID, func);

	// reuse annotation info when checking for overflow
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

	addPushf(pushf_i, pushr1_i);
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, pushf_i);
	pushf_i->SetFallthrough(pushr1_i);
	pushf_i->SetComment("-- carefullyInsertBefore NoFlagRegPlusReg");

	addPushRegister(pushr1_i, p_reg1, addRR_i);

	addAddRegisters(addRR_i, p_reg1, p_reg2, popr1_i);
	addRR_i->SetComment(msg);

	addPopRegister(popr1_i, p_reg1, popf_i);
	addPopf(popf_i, originalInstrumentInstr);

	addOverflowCheck(addRR_i, addRR_annot, p_policy, originalAddress);
}

void IntegerTransform::addOverflowCheckNoFlag_RegPlusConstant(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const int p_constantValue, const Register::RegisterName& p_reg3, int p_policy)
{
//	cerr << "integertransform: doit: reg+constant: register: " << Register::toString(p_reg1) << " constant: " << dec << p_constantValue << " target register: " << Register::toString(p_reg3) << "  annotation: " << p_annotation.toString() << endl;

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
	//   push r3                ;     save register
	//   pushf                  ;     save flags
	//   mov r3, r1             ;     r3 = r1
	//   add r3, constant       ;     r3 = r1 + constant;
	//   <check for overflow>   ;     reuse overflow code
	//   popf                   ;     restore flags
	//   pop r3                 ;     restore register
	//
	//   lea r3, [r1+constant]  ;     original instruction        
	//
	// Note: if r3 == r1, code still works (though inefficiently)
	//

	db_id_t fileID = p_instruction->GetAddress()->GetFileID();
	Function_t* func = p_instruction->GetFunction();

	Instruction_t* pushR3_i = allocateNewInstruction(fileID, func);
	Instruction_t* pushf_i = allocateNewInstruction(fileID, func);
	Instruction_t* movR3R1_i = allocateNewInstruction(fileID, func);
	Instruction_t* addR3Constant_i = allocateNewInstruction(fileID, func);
	Instruction_t* popf_i = allocateNewInstruction(fileID, func);
	Instruction_t* popR3_i = allocateNewInstruction(fileID, func);

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

	addPushRegister(pushR3_i, p_reg3, pushf_i);
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, pushR3_i);
	pushR3_i->SetComment("in lea -- RegPlusConstant");
	pushR3_i->SetFallthrough(pushf_i);  
	addPushf(pushf_i, movR3R1_i);

	addMovRegisters(movR3R1_i, p_reg3, p_reg1, addR3Constant_i);
	addAddRegisterConstant(addR3Constant_i, p_reg3, p_constantValue, popf_i);
	addPopf(popf_i, popR3_i);
	addPopRegister(popR3_i, p_reg3, originalInstrumentInstr);

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
	//   push r3                ;     save register
	//   pushf                  ;     save flags
	//   mov r3, r1             ;     r3 = r1
	//   imul r3, constant      ;     r3 = r1 * constant;
	//   <check for overflow>   ;     reuse overflow code
	//   popf                   ;     restore flags
	//   pop r3                 ;     restore register
	//   lea r3, [r1*constant]          
	//
	// Note: if r3 == r1, code still works (though inefficiently)
	//
	db_id_t fileID = p_instruction->GetAddress()->GetFileID();
	Function_t* func = p_instruction->GetFunction();

	Instruction_t* pushR3_i = allocateNewInstruction(fileID, func);
	Instruction_t* pushf_i = allocateNewInstruction(fileID, func);
	Instruction_t* movR3R1_i = allocateNewInstruction(fileID, func);
	Instruction_t* mulR3Constant_i = allocateNewInstruction(fileID, func);
	Instruction_t* popf_i = allocateNewInstruction(fileID, func);
	Instruction_t* popR3_i = allocateNewInstruction(fileID, func);

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

	addPushRegister(pushR3_i, p_reg3, pushf_i);
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, pushR3_i);
	pushR3_i->SetFallthrough(pushf_i);
	pushR3_i->SetComment("in lea -- Reg * Constant"); 
	addPushf(pushf_i, movR3R1_i);

	addMovRegisters(movR3R1_i, p_reg3, p_reg1, mulR3Constant_i);
	addMulRegisterConstant(mulR3Constant_i, p_reg3, p_constantValue, popf_i);
	addPopf(popf_i, popR3_i);
	addPopRegister(popR3_i, p_reg3, originalInstrumentInstr);

	mulR3Constant_i->SetComment(msg);
	addOverflowCheck(mulR3Constant_i, mulR3Constant_annot, p_policy, originalAddress);
}

void IntegerTransform::handleTruncation(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	if (p_annotation.getTruncationFromWidth() == 32)
	{
		if (p_annotation.getTruncationToWidth() == 8 || p_annotation.getTruncationToWidth() == 16)
		{
			addTruncationCheck(p_instruction, p_annotation, p_policy);
		}
		else
		{
			cerr << "integertransform: TRUNCATION annotation not yet handled: " << p_annotation.toString() << "fromWidth: " << p_annotation.getTruncationFromWidth() << " toWidth: " << p_annotation.getTruncationToWidth() << endl;
		}
	}
}


//
// before:       after:
// <inst>        nop (with callback handler)
//               <inst>
//                     
void IntegerTransform::handleInfiniteLoop(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	assert(getFileIR() && p_instruction);

    db_id_t fileID = p_instruction->GetAddress()->GetFileID();
    Function_t* func = p_instruction->GetFunction();

	AddressID_t *originalAddress = p_instruction->GetAddress();
	Instruction_t* nop_i = allocateNewInstruction(fileID, func);

	addNop(nop_i, p_instruction);
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, nop_i);

	addCallbackHandler(string(INFINITE_LOOP_DETECTOR), originalInstrumentInstr, nop_i, nop_i->GetFallthrough(), p_policy, originalAddress);
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
	assert(getFileIR() && p_instruction && p_instruction->GetFallthrough());

	Register::RegisterName targetReg = getTargetRegister(p_instruction);
	if (targetReg == Register::UNKNOWN)
	{
		cerr << "integertransform: OVERFLOW UNKNOWN SIGN: unknown register -- skip instrumentation" << endl;
		return;
	}

cerr << "IntegerTransform::addOverflowCheck(): instr: " << p_instruction->getDisassembly() << " address: " << std::hex << p_instruction->GetAddress() << " annotation: " << p_annotation.toString() << " policy: " << p_policy << endl;
	
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
		cerr << "integertransform: SIGNED OVERFLOW: " << detector << endl;
	}

	jncond_i->SetDataBits(dataBits);
	jncond_i->SetComment(jncond_i->getDisassembly());
	jncond_i->SetTarget(nextOrig_i); 

	p_instruction->SetFallthrough(jncond_i); 

	if (p_addressOriginalInstruction)
	{
		addCallbackHandler(detector, p_instruction, jncond_i, nextOrig_i, p_policy, p_addressOriginalInstruction);
	}
	else if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
	{
		// implement saturating arithmetic, e.g.:
		// mov <reg>, value
		Instruction_t* saturate_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());

		if (p_annotation.flowsIntoCriticalSink() && p_annotation.getBitWidth() == 32)
		{
			cerr << "integertransform: OVERFLOW UNSIGNED 32: CRITICAL SINK: saturate by masking" << endl;

			db_id_t fileID = p_instruction->GetAddress()->GetFileID();
			Function_t* func = p_instruction->GetFunction();
			Instruction_t* pushf_i = allocateNewInstruction(fileID, func);
			Instruction_t* popf_i = allocateNewInstruction(fileID, func);
			addCallbackHandler(detector, p_instruction, jncond_i, pushf_i, p_policy);
			addPushf(pushf_i, saturate_i);
			addAndRegister32Mask(saturate_i, targetReg, 0x00FFFFFF, popf_i);
			addPopf(popf_i, nextOrig_i);
		}
		else
		{
			addCallbackHandler(detector, p_instruction, jncond_i, saturate_i, p_policy);
			addMaxSaturation(saturate_i, targetReg, p_annotation, nextOrig_i);
		}
	}
	else
	{
		addCallbackHandler(detector, p_instruction, jncond_i, nextOrig_i, p_policy);
	}

	getFileIR()->GetAddresses().insert(jncond_a);
	getFileIR()->GetInstructions().insert(jncond_i);
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
	assert(getFileIR() && p_instruction);

	Register::RegisterName targetReg = getTargetRegister(p_instruction);
	if (targetReg == Register::UNKNOWN)
	{
		cerr << "IntegerTransform::addUnderflowCheck(): instr: " << p_instruction->getDisassembly() << " address: " << p_instruction->GetAddress() << " annotation: " << p_annotation.toString() << "-- SKIP b/c no target registers" << endl;
		return;
	}
	
	cerr << "IntegerTransform::addUnderflowCheck(): instr: " << p_instruction->getDisassembly() << " address: " << p_instruction->GetAddress() << " annotation: " << p_annotation.toString() << endl;

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
		cerr << "integertransform: UNSIGNED OVERFLOW: " << detector << endl;
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

		cerr << "integertransform: SIGNED UNDERFLOW: " << detector << endl;
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
		cerr << "integertransform: UNDERFLOW UNKONWN: assume signed for now: " << detector << endl;
	}

#ifdef XXX
	if (p_annotation.getBitWidth() == 8 && (p_annotation.isUnsigned() || p_annotation.isUnknownSign()))
	{
		// special case 8-bit unknown or unsigned underflows b/c of gcc codegen
		if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
		{
			cerr << "integertransform: 8-bit UNDERFLOW: gcc (saturation disabled): " << endl;
			p_policy = POLICY_DEFAULT;
		}
	}
#endif


	jncond_i->SetDataBits(dataBits);
	jncond_i->SetComment(jncond_i->getDisassembly());
	jncond_i->SetTarget(nextOrig_i); 

	p_instruction->SetFallthrough(jncond_i); 

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

	getFileIR()->GetAddresses().insert(jncond_a);
	getFileIR()->GetInstructions().insert(jncond_i);
}


//
// STARS has extensive discussion of the logic behind truncation and
//  signedness annotations in module SMPInstr.cpp, method EmitIntegerErrorAnnotation().
//  The possible combinations are categorized in terms of the instrumentation
//  needed at run time:
//  CHECK TRUNCATION UNSIGNED : discarded bits must be all zeroes.
//  CHECK TRUNCATION SIGNED: discarded bits must be sign-extension of stored bits.
//  CHECK TRUNCATION UNKNOWNSIGN: discarded bits must be all zeroes, OR must
//   be the sign-extension of the stored bits.
// All truncation annotations are emitted on stores (mov opcodes in x86).
//  Other possibilities might be handled in the future.
//
void IntegerTransform::addTruncationCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	assert(getFileIR() && p_instruction);
	assert(p_annotation.getTruncationFromWidth() == 32 && p_annotation.getTruncationToWidth() == 8 || p_annotation.getTruncationToWidth() == 16);

	cerr << "IntegerTransform::addTruncationCheck(): instr: " << p_instruction->getDisassembly() << " address: " << p_instruction->GetAddress() << " annotation: " << p_annotation.toString() << " policy: " << p_policy << endl;
	string detector; 

	// Complicated case:
	// 80484ed 3 INSTR CHECK TRUNCATION UNKNOWNSIGN 32 EAX 8 AL ZZ mov     [ebp+var_4], al
	//  example: for unknownsign truncation - 8 bit on AL
	//          it's ok if 24 upper bits are sign-extension or all 0's
	//          Re-phrased: upper 24 are 0s, upper 25 are 0s, upper 25 are 1s
	//           are all OK in the UNKNOWNSIGN case
	//          Note that upper 24 are 0's means no need to check for the
	//           case where the upper 25 are 0's; already OK.
	//
	//             <save flags>
	//             test eax, 0xFFFFFF00   ; (for 8 bit) 
	//             jz <continue>          ; upper 24 bits are 0's 
	//
	//             cmp eax, 0xFFFFFF80    ;(for 8 bit) 
	//             jae continue           ; upper 25 bits are 1's
	//             (invoke truncation handler)
	//             nop ; truncation handler returns here
	//      SAT:   saturating-arithmetic  ; optional
	//
	// continue:   <restore flags>
	//             mov [ebp+var_4], al    ; <originalInstruction>
	//

	// Unsigned case:
	// 80484ed 3 INSTR CHECK TRUNCATION UNSIGNED 32 EAX 8 AL ZZ mov     [ebp+var_4], al
	//  example: for unsigned truncation - 8 bit on AL
	//          it's ok if 24 upper bits are all 0's
	//
	//             <save flags>
	//             test eax, 0xFFFFFF00   ; (for 8 bit) 
	//             jz <continue>          ; upper 24 bits are 0's 
	//
	//             (invoke truncation handler)
	//             nop ; truncation handler returns here
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
	Instruction_t* saturate_i = NULL;
	Instruction_t* popf_i = allocateNewInstruction(fileID, func);

	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC) {
		saturate_i = allocateNewInstruction(fileID, func);
		addMaxSaturation(saturate_i, p_annotation.getRegister(), p_annotation, popf_i);
	}

	addPushf(pushf_i, test_i);
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, pushf_i);
	pushf_i->SetFallthrough(test_i); 
	pushf_i->SetComment("-- in truncation");
	originalInstrumentInstr->SetComment("-- in truncation (was original)");

	unsigned mask = 0;
	unsigned mask2 = 0;
	if (p_annotation.getTruncationToWidth() == 16) {
		mask = 0xFFFF0000;
		mask2 = 0xFFFF8000;
		if (p_annotation.isUnsigned())
			detector = string(TRUNCATION_DETECTOR_UNSIGNED_32_16);
		else if (p_annotation.isSigned())
			detector = string(TRUNCATION_DETECTOR_SIGNED_32_16);
		else
			detector = string(TRUNCATION_DETECTOR_32_16);
	}
	else if (p_annotation.getTruncationToWidth() == 8) {
		mask = 0xFFFFFF00;
		mask2 = 0xFFFFFF80;
		if (p_annotation.isUnsigned())
			detector = string(TRUNCATION_DETECTOR_UNSIGNED_32_8);
		else if (p_annotation.isSigned())
			detector = string(TRUNCATION_DETECTOR_SIGNED_32_8);
		else
			detector = string(TRUNCATION_DETECTOR_32_8);
	}
         
	if (p_annotation.isUnsigned()) {
		addTestRegisterMask(test_i, p_annotation.getRegister(), mask, jz_i);
		Instruction_t* nop_i = allocateNewInstruction(fileID, func);
		addJz(jz_i, nop_i, popf_i); // target = popf_i, fall-through = nop_i

		if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC) {
			addNop(nop_i, saturate_i);
			nop_i->SetComment("UNSIGNED TRUNC: fallthrough: saturating arithmetic instruction");
			addCallbackHandler(detector, originalInstrumentInstr, nop_i, saturate_i, p_policy, saveAddress);
		}
		else {
			addNop(nop_i, popf_i);
			nop_i->SetComment(string("UNSIGNED TRUNC: fallthrough: popf"));
			addCallbackHandler(detector, originalInstrumentInstr, nop_i, popf_i, p_policy, saveAddress);
		}
	}
	else {
		// Signed and unknown signed cases are almost identical:
		// 80484ed 3 INSTR CHECK TRUNCATION SIGNED 32 EAX 8 AL ZZ mov     [ebp+var_4], al
		// 80484ed 3 INSTR CHECK TRUNCATION UNKNOWNSIGN 32 EAX 8 AL ZZ mov     [ebp+var_4], al
		//  example: for signed truncation - 8 bit on AL
		//          it's ok if 25 upper bits are all 1's or all 0's
		//  example: for unknownsign truncation - 8 bit on AL
		//          it's ok if 25 upper bits are all 1's or 24 upper bits all 0's
		//
		//             <save flags>
		//             test eax, 0xFFFFFF80   ; (for 8 bit) 0xFFFFFF00 for UNKNOWN
		//             jz <continue>          ; upper 25 bits are 0's 
		//
		//             cmp eax, 0xFFFFFF80    ;(for 8 bit) 
		//             jae continue           ; upper 25 bits are 1's
		//             (invoke truncation handler) 
		//             nop ; truncation handler callback here
		//      SAT:   saturating-arithmetic  ; optional
		//
		// continue:   <restore flags>
		//             mov [ebp+var_4], al    ; <originalInstruction>
		//

		if (p_annotation.isSigned()) {
			addTestRegisterMask(test_i, p_annotation.getRegister(), mask2, jz_i);
		}
		else { // must be UNKNOWNSIGN
			addTestRegisterMask(test_i, p_annotation.getRegister(), mask, jz_i);
		}
		Instruction_t* nop_i = allocateNewInstruction(fileID, func);
		Instruction_t* s_cmp_i = allocateNewInstruction(fileID, func);
		Instruction_t* s_jae_i = allocateNewInstruction(fileID, func);

		addJz(jz_i, s_cmp_i, popf_i); // target = popf_i, fall-through = s_cmp_i
		jz_i->SetComment(string("jz - SIGNED or UNKNOWNSIGN TRUNC"));
		addCmpRegisterMask(s_cmp_i, p_annotation.getRegister(), mask2, s_jae_i);
		addJae(s_jae_i, nop_i, popf_i); // target = popf_i, fall-through = nop_i
		s_jae_i->SetComment(string("jae - SIGNED or UNKNOWNSIGN TRUNC"));

		if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC) {
			addNop(nop_i, saturate_i);
			nop_i->SetComment("SIGNED or UNKNOWNSIGN TRUNC: fallthrough: saturating arithmetic instruction");
			addCallbackHandler(detector, originalInstrumentInstr, nop_i, saturate_i, p_policy, saveAddress);
		}
		else {
			addNop(nop_i, popf_i);
			nop_i->SetComment(string("SIGNED or UNKNOWNSIGN TRUNC: fallthrough: popf"));
			addCallbackHandler(detector, originalInstrumentInstr, nop_i, popf_i, p_policy, saveAddress);
		}
	} // end of SIGNED and UNKNOWNSIGN case
	addPopf(popf_i, originalInstrumentInstr); // common to all cases

	//    cerr << "addTruncationCheck(): --- END ---" << endl;
}

void IntegerTransform::addMaxSaturation(Instruction_t *p_instruction, Register::RegisterName p_reg, const MEDS_InstructionCheckAnnotation& p_annotation, Instruction_t *p_fallthrough)
{
	assert(getFileIR() && p_instruction);

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

void IntegerTransform::addSaturation(Instruction_t *p_instruction, Register::RegisterName p_reg, unsigned p_value, const MEDS_InstructionCheckAnnotation& p_annotation, Instruction_t *p_fallthrough)
{
	assert(getFileIR() && p_instruction);

	p_instruction->SetFallthrough(p_fallthrough);

	addMovRegisterUnsignedConstant(p_instruction, p_reg, p_value, p_fallthrough);
}

void IntegerTransform::addZeroSaturation(Instruction_t *p_instruction, Register::RegisterName p_reg, Instruction_t *p_fallthrough)
{
	assert(getFileIR() && p_instruction);

	p_instruction->SetFallthrough(p_fallthrough);

	addMovRegisterUnsignedConstant(p_instruction, p_reg, 0, p_fallthrough);
}

bool IntegerTransform::isBlacklisted(Function_t *func)
{
	if (!func) return false;

	const char *funcName = func->GetName().c_str();
	if (strcasestr(funcName, "hash") ||
		strcasestr(funcName, "compress") ||
		strcasestr(funcName, "encode") ||
		strcasestr(funcName, "decode") ||
		strcasestr(funcName, "crypt") ||
		strcasestr(funcName, "yyparse") ||
		strcasestr(funcName, "yyerror") ||
		strcasestr(funcName, "yydestruct") ||
		strcasestr(funcName, "yyrestart") ||
		strcasestr(funcName, "yylex") ||
		strcasestr(funcName, "yy_") ||
		strcasestr(funcName, "random"))
	{
		return true;
	}
	else
		return false;
}

//
//      <instruction to instrument>
//      jno <originalFallthroughInstruction> 
//      jnc <originalFallthroughInstruction> 
//      nop [attach callback handler]
//      saturate target register (if policy dictates)
//      <originalFallthroughInstruction>
//
void IntegerTransform::addOverflowCheckUnknownSign(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	assert(getFileIR() && p_instruction && p_instruction->GetFallthrough());
	Register::RegisterName targetReg = getTargetRegister(p_instruction);
	if (targetReg == Register::UNKNOWN)
	{
		cerr << "integertransform: OVERFLOW UNKNOWN SIGN: unknown register -- skip instrumentation" << endl;
		return;
	}

cerr << "IntegerTransform::addOverflowCheckUnknownSign(): instr: " << p_instruction->getDisassembly() << " address: " << std::hex << p_instruction->GetAddress() << " annotation: " << p_annotation.toString() << " policy: " << p_policy << endl;
	
	string detector(OVERFLOW_UNKNOWN_SIGN_DETECTOR);
	string dataBits;

	// for now assume we're dealing with add/sub 32 bit
	db_id_t fileID = p_instruction->GetAddress()->GetFileID();
	Function_t* func = p_instruction->GetFunction();

	Instruction_t* jno_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());
	Instruction_t* jnc_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());
	Instruction_t* nop_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());

	// save fallthrough from original instruction
	Instruction_t* nextOrig_i = p_instruction->GetFallthrough();

	// instrument for both jno and jnc
	p_instruction->SetFallthrough(jno_i); 
	addJno(jno_i, jnc_i, nextOrig_i);
	addJnc(jnc_i, nop_i, nextOrig_i);
	addNop(nop_i, nextOrig_i);

	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC) 
	{
		Instruction_t* saturate_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());
		addCallbackHandler(detector, p_instruction, nop_i, saturate_i, p_policy);
		addMaxSaturation(saturate_i, targetReg, p_annotation, nextOrig_i);
	}
	else 
	{
		addCallbackHandler(detector, p_instruction, nop_i, nextOrig_i, p_policy);
	}
}
