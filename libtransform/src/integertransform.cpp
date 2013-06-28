#include <assert.h>
#include "integertransform.hpp"
#include "leapattern.hpp"

// 
// For list of blacklisted functions, see: isBlacklisted()
//

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

		if (isBlacklisted(func))
		{
			cerr << "Heuristic filter: " << func->GetName() << endl;
			continue;
		}

		logMessage(__func__, "processing fn: " + func->GetName());

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

				logMessage(__func__, annotation, "-- instruction: " + insn->getDisassembly());

				if (annotation.isIdiom())
				{
					logMessage(__func__, "skip IDIOM");
					continue;
				}

				if (!insn->GetFallthrough())
				{
					logMessage(__func__, "Warning: no fall through for instruction -- skipping");
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
						logMessage(__func__, "annotation has unknown sign: skipping");
						continue;
					}
					handleSignedness(insn, annotation, policy);
				}
				else if (annotation.isInfiniteLoop())
				{
					handleInfiniteLoop(insn, annotation, POLICY_EXIT);
				}
				else
				{
					logMessage(__func__, "unknown annotation");
				}
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
		logMessage(__func__, "case not yet handled");
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
		logMessage(__func__, "unexpected bit width and register combination: skipping");
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
	if (p_annotation.isUnknownSign() && !p_annotation.isNoFlag())
	{
		addOverflowCheckUnknownSign(p_instruction, p_annotation, p_policy);
	}
	else if (p_annotation.isNoFlag())
	{
		// handle lea
		addOverflowCheckNoFlag(p_instruction, p_annotation, p_policy);
	}
	else if (isMultiplyInstruction(p_instruction) || p_annotation.isUnderflow() || p_annotation.isOverflow())
	{
		// handle signed/unsigned add/sub overflows (non lea)
		addOverflowCheck(p_instruction, p_annotation, p_policy);
	}
	else
	{
		logMessage(__func__, "OVERFLOW type not yet handled");
	}
}

void IntegerTransform::handleUnderflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	if (p_annotation.isUnderflow())
	{
		addUnderflowCheck(p_instruction, p_annotation, p_policy);
	}
	else
	{
		logMessage(__func__, "UNDERFLOW type not yet handled");
	}
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
		logMessage(__func__, "invalid or unhandled lea pattern - skipping: ");
		return;
	}
	
	if (leaPattern.getRegister1() == Register::UNKNOWN ||
		leaPattern.getRegister1() == Register::ESP ||
		leaPattern.getRegister1() == Register::EBP)
	{
		logMessage(__func__, "destination register is unknown, esp or ebp -- skipping: ");
		return;
	}
	
	if (leaPattern.isRegisterPlusRegister())
	{
		Register::RegisterName reg1 = leaPattern.getRegister1();
		Register::RegisterName reg2 = leaPattern.getRegister2();
		Register::RegisterName target = getTargetRegister(p_instruction);

		if (reg1 == Register::UNKNOWN || reg2 == Register::UNKNOWN || target == Register::UNKNOWN)
		{
			logMessage(__func__, "lea reg+reg pattern: error retrieving register: reg1: " + Register::toString(reg1) + " reg2: " + Register::toString(reg2) + " target: " + Register::toString(target));
			return;
		}
		else if (reg2 == Register::ESP) 
		{
			logMessage(__func__, "source register is esp -- skipping: ");
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
			logMessage(__func__, "lea reg+neg constant pattern: skip this annotation type (prone to false positives)");
			return;
		}
		else if (reg1 == Register::UNKNOWN || target == Register::UNKNOWN)
		{
			logMessage(__func__, "lea reg+constant pattern: error retrieving register: reg1: " + Register::toString(reg1) + " target: " + Register::toString(target));
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
			logMessage(__func__, "lea reg*constant pattern: error retrieving register: reg1: " + Register::toString(reg1) + " target: " + Register::toString(target));
			return;
		}
		else
		{
			addOverflowCheckNoFlag_RegTimesConstant(p_instruction, p_annotation, reg1, value, target, p_policy);
		}
	}
	else
	{
		logMessage(__func__, "pattern not yet handled");
		return;
	}
}

// Example annotation to handle
// 804852e      3 INSTR CHECK OVERFLOW NOFLAGSIGNED 32 EDX+EAX ZZ lea     eax, [edx+eax] Reg1: EDX Reg2: EAX
void IntegerTransform::addOverflowCheckNoFlag_RegPlusReg(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const Register::RegisterName& p_reg2, const Register::RegisterName& p_reg3, int p_policy)
{
	cerr << __func__ << ": reg+constant: r1: " << Register::toString(p_reg1) << " r2: " << Register::toString(p_reg2) << " target register: " << Register::toString(p_reg3) << "  annotation: " << p_annotation.toString() << endl;

	//   orig:  lea r3, [r1+r2]
	//   <originalNext>
	//
	// Instrumentation:
	//   push r1                ;   save r1
	//   pushf                  ;   save flags
	//   add r1, r2             ;   r1 = r1 + r2
	//        <overflowcheck>   ;   check for overflow as dictated by annotation
	//          (jno|jnc <restore>)    ; SIGNED|UNSIGNED
	//            fallthrough--><saturate>
	//          (jno&jnc <restore>)    ; UNKNOWNSIGN both flags  
	//            fallthrough--><saturate>
	//
	// <restore>
	//          popf                   ; restore flags
	//          pop r1                 ; restore register
	//
	// <orig>:  lea r3, [r1+r2]        ; original instruction        
	//         <originalNext>          ; original next instruction
	//
	// <saturate>                      ; optional saturation code
	//         popf                    ; restore flags
	//         pop r1                  ; restore register
	//         saturateMax(r3)         ; 
	//            fallthrough-->originalNext
	//

	db_id_t fileID = p_instruction->GetAddress()->GetFileID();
	Function_t* func = p_instruction->GetFunction();

	Instruction_t* pushr1_i = allocateNewInstruction(fileID, func);
	Instruction_t* pushf_i = allocateNewInstruction(fileID, func);
	Instruction_t* addRR_i = allocateNewInstruction(fileID, func);
	Instruction_t* popf_i = allocateNewInstruction(fileID, func);
	Instruction_t* popr1_i = allocateNewInstruction(fileID, func);

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
	Instruction_t* originalNextInstr = p_instruction->GetFallthrough();
	AddressID_t *originalAddress = p_instruction->GetAddress();

	addPushRegister(pushr1_i, p_reg1, pushf_i);
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, pushr1_i);
	pushr1_i->SetFallthrough(pushf_i);
	pushr1_i->SetComment("-- carefullyInsertBefore NoFlagRegPlusReg");

	addPushf(pushf_i, addRR_i);

	addAddRegisters(addRR_i, p_reg1, p_reg2, popf_i);
	addRR_i->SetComment(msg);

	addPopf(popf_i, popr1_i);
	addPopRegister(popr1_i, p_reg1, originalInstrumentInstr);

	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
	{
	//   add r1, r2             ;   r1 = r1 + r2
	//          (jno|jnc <restore>)    ; SIGNED|UNSIGNED
	//            fallthrough--><saturate>
	//          (jno&jnc <restore>)    ; UNKNOWNSIGN both flags  
	//            fallthrough--><saturate>
	//
	// <restore>
	//          popf                   ; restore flags
	//          pop r1                 ; restore register
	//
	// <orig>:  lea r3, [r1+r2]        ; original instruction        
	//         <originalNext>          ; original next instruction
	//
	// <saturate>                      ; optional saturation code
	//         popf                    ; restore flags
	//         pop r1                  ; restore register
	//         saturateMax(r3)         ; 
	//            fallthrough-->originalNext

		Instruction_t* j_i = allocateNewInstruction(fileID, func);
		Instruction_t* jnc_i = NULL;
		Instruction_t* popfsat_i = allocateNewInstruction(fileID, func);
		Instruction_t* popR1sat_i = allocateNewInstruction(fileID, func);
		Instruction_t* saturate_i = allocateNewInstruction(fileID, func);

		addRR_i->SetFallthrough(j_i);
	
		if (p_annotation.isSigned())
		{
			addJno(j_i, popfsat_i, popf_i);
		}
		else if (p_annotation.isUnsigned())
		{
			addJnc(j_i, popfsat_i, popf_i);
		}
		else
		{
			jnc_i = allocateNewInstruction(fileID, func);
			addJno(j_i, jnc_i, popf_i);
			addJnc(jnc_i, popfsat_i, popf_i);
		}

		addOverflowCheckForLea(addRR_i, addRR_annot, p_policy, originalAddress);

		addPopf(popfsat_i, popR1sat_i);
		addPopRegister(popR1sat_i, p_reg1, saturate_i);
		addMaxSaturation(saturate_i, p_reg3, p_annotation, originalNextInstr);
	}
	else
	{
		addOverflowCheckForLea(addRR_i, addRR_annot, p_policy, originalAddress);
	}
}

void IntegerTransform::addOverflowCheckNoFlag_RegPlusConstant(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const int p_constantValue, const Register::RegisterName& p_reg3, int p_policy)
{
	if (!p_instruction)
		return;

	if (p_instruction->GetIndirectBranchTargetAddress())
		cerr << "IBTA set: ";
	else
		cerr << "no IBTA: ";

	cerr << __func__ << ": reg+constant: register: " << Register::toString(p_reg1) << " constant: " << dec << p_constantValue << " target register: " << Register::toString(p_reg3) << "  annotation: " << p_annotation.toString() << endl;

	//
	// Original instruction is of the form:
	//   lea r3, [r1+constant]          
	//   lea r3, [r1-constant]          
	//   <originalNext>
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
	//          push r3                ; save register
	//          pushf                  ; save flags
	//          mov r3, r1             ; r3 = r1
	//          add r3, constant       ; r3 = r1 + constant;
	//              <overflowCheck>    ; reuse overflow code
	//          (jno|jnc <restore>)    ; SIGNED|UNSIGNED
	//            fallthrough--><saturate>
	//          (jno&jnc <restore>)    ; UNKNOWNSIGN both flags  
	//            fallthrough--><saturate>
	// <restore>
	//          popf                   ; restore flags
	//          pop r3                 ; restore register
	//
	//          
	// <orig>:  lea r3, [r1+constant]  ; original instruction        
	//         <originalNext>          ; original next instruction
	//                                 
	// <saturate>                      ; optional saturation code
	//         popf                    ; restore flags
	//         pop r3                  ; restore register
	//         saturateMax(r3)         ; 
	//            fallthrough-->originalNext
	//
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
	Instruction_t* originalNextInstr = p_instruction->GetFallthrough();

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
	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
	{
	//          (jno|jnc <restore>)    ; SIGNED|UNSIGNED
	//            fallthrough--><saturate>
	//          (jno&jnc <restore>)    ; UNKNOWNSIGN both flags  
	//            fallthrough--><saturate>
	//
	// <restore>
	//          popf                   ; restore flags
	//          pop r3                 ; restore register
	//
	// <saturate>
	//         popf                   ; restore flags
	//         pop r3                 ; restore register
	//         saturateMax(r3)        ; 
	//            fallthrough-->originalNext
		Instruction_t* j_i = allocateNewInstruction(fileID, func);
		Instruction_t* jnc_i = NULL;
		Instruction_t* popfsat_i = allocateNewInstruction(fileID, func);
		Instruction_t* popR3sat_i = allocateNewInstruction(fileID, func);
		Instruction_t* saturate_i = allocateNewInstruction(fileID, func);

		addR3Constant_i->SetFallthrough(j_i);
	
		if (p_annotation.isSigned())
		{
			addJno(j_i, popfsat_i, popf_i);
		}
		else if (p_annotation.isUnsigned())
		{
			addJnc(j_i, popfsat_i, popf_i);
		}
		else
		{
			jnc_i = allocateNewInstruction(fileID, func);
			addJno(j_i, jnc_i, popf_i);
			addJnc(jnc_i, popfsat_i, popf_i);
		}

		addOverflowCheckForLea(addR3Constant_i, addR3Constant_annot, p_policy, originalAddress);

		addPopf(popfsat_i, popR3sat_i);
		addPopRegister(popR3sat_i, p_reg3, saturate_i);
		addMaxSaturation(saturate_i, p_reg3, p_annotation, originalNextInstr);
	}
	else
	{
		addOverflowCheckForLea(addR3Constant_i, addR3Constant_annot, p_policy, originalAddress);
	}
}

void IntegerTransform::addOverflowCheckNoFlag_RegTimesConstant(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const int p_constantValue, const Register::RegisterName& p_reg3, int p_policy)
{
	cerr << __func__ << ": reg*constant: register: " << Register::toString(p_reg1) << " constant: " << p_constantValue << " target register: " << Register::toString(p_reg3) << "  annotation: " << p_annotation.toString() << endl;

	//
	// Original instruction is of the form:
	//   lea r3, [r1*constant]          
	//   <originalNext>
	//
	// Instrumentation:
	//         push r3                ; save register
	//         pushf                  ; save flags
	//         mov r3, r1             ; r3 = r1
	//         imul r3, constant      ; r3 = r1 * constant;
	//           <overflowCheck>      ; emit diagnostics
	//         (jo <saturate>)        ; optional saturation code
	//         popf                   ; restore flags
	//         pop r3                 ; restore register
	//
	// <orig>: lea r3, [r1*constant]  ; original instruction        
	//         <originalNext>         ; original next instruction
    //
	// ; optional saturation code
	// <saturate>
	//         popf                   ; restore flags
	//         pop r3                 ; restore register
	//         saturateMax(r3)        ; 
	//            fallthrough-->originalNext
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
	Instruction_t* originalNextInstr = p_instruction->GetFallthrough();

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

	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
	{
	//         (jo <saturate>)        ; optional saturation code
	//         popf                   ; restore flags
	//
	// <saturate>
	//         popf                   ; restore flags
	//         pop r3                 ; restore register
	//         saturateMax(r3)        ; 
	//            fallthrough-->originalNext
		Instruction_t* jo_i = allocateNewInstruction(fileID, func);
		Instruction_t* popfsat_i = allocateNewInstruction(fileID, func);
		Instruction_t* popR3sat_i = allocateNewInstruction(fileID, func);
		Instruction_t* saturate_i = allocateNewInstruction(fileID, func);

		mulR3Constant_i->SetFallthrough(jo_i);
		addOverflowCheckForLea(mulR3Constant_i, mulR3Constant_annot, p_policy, originalAddress);
	
		addJo(jo_i, popf_i, popfsat_i);
		addPopf(popfsat_i, popR3sat_i);
		addPopRegister(popR3sat_i, p_reg3, saturate_i);
		addMaxSaturation(saturate_i, p_reg3, p_annotation, originalNextInstr);
	}
	else
	{
		// fallthrough was set previously to popf_i
		addOverflowCheckForLea(mulR3Constant_i, mulR3Constant_annot, p_policy, originalAddress);
	}
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
			cerr << __func__ << ": TRUNCATION annotation not yet handled: " << p_annotation.toString() << "fromWidth: " << p_annotation.getTruncationFromWidth() << " toWidth: " << p_annotation.getTruncationToWidth() << endl;
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

	logMessage(__func__, "handling infinite loop");

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
// p_addressOriginalInstruction is set when we call method from lea instrumentation
void IntegerTransform::addOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	assert(getFileIR() && p_instruction && p_instruction->GetFallthrough());
	
	Register::RegisterName targetReg = getTargetRegister(p_instruction);
	if (targetReg == Register::UNKNOWN) 
	{
		logMessage(__func__, "OVERFLOW UNKNOWN SIGN: unknown register -- skip instrumentation");
		return;
	}

cerr << __func__ <<  ": instr: " << p_instruction->getDisassembly() << " address: " << std::hex << p_instruction->GetAddress() << " annotation: " << p_annotation.toString() << " policy: " << p_policy << endl;

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

	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
	{
		// implement saturating arithmetic, e.g.:
		// mov <reg>, value
		Instruction_t* saturate_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());

		if (p_annotation.flowsIntoCriticalSink() && p_annotation.getBitWidth() == 32)
		{
			logMessage(__func__, "OVERFLOW UNSIGNED 32: CRITICAL SINK: saturate by masking");

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

	cerr << __func__ << ": instr: " << p_instruction->getDisassembly() << " address: " << p_instruction->GetAddress() << " annotation: " << p_annotation.toString() << " policy: " << p_policy << endl;

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
	if (/*strcasestr(funcName, "hash") ||
		strcasestr(funcName, "compress") ||
		strcasestr(funcName, "encode") ||
		strcasestr(funcName, "decode") ||
		strcasestr(funcName, "crypt") ||
		*/
		//          haystack, needle
		strcasestr(funcName, "yyparse") ||
		strcasestr(funcName, "yyerror") ||
		strcasestr(funcName, "yydestruct") ||
		strcasestr(funcName, "yyrestart") ||
		strcasestr(funcName, "yylex") ||
		strcasestr(funcName, "yy_")
		/* ||
		strcasestr(funcName, "sub_17810") ||
		strcasestr(funcName, "process_active_connection") 
		strcasestr(funcName, "random")
		*/
		)
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

cerr << __func__ << ": instr: " << p_instruction->getDisassembly() << " address: " << std::hex << p_instruction->GetAddress() << " annotation: " << p_annotation.toString() << " policy: " << p_policy << endl;
	
	// set detector/handler
	string detector(OVERFLOW_UNKNOWN_SIGN_DETECTOR);
	if (!isMultiplyInstruction(p_instruction))
	{
		if (p_annotation.getBitWidth() == 32)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_UNKNOWN_32);
		else if (p_annotation.getBitWidth() == 16)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_UNKNOWN_16);
		else if (p_annotation.getBitWidth() == 8)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_UNKNOWN_8);
	}

	// for now assume we're dealing with add/sub 32 bit
	db_id_t fileID = p_instruction->GetAddress()->GetFileID();
	Function_t* func = p_instruction->GetFunction();

	Instruction_t* jno_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());
	Instruction_t* jnc_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());
	Instruction_t* nop_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());

	// save fallthrough from original instruction
	Instruction_t* nextOrig_i = p_instruction->GetFallthrough();

	// instrument for both jno and jnc
	// redundant for imul, but that's ok, optimize later
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

//
//      <instruction to instrument>
//      jno|jnc <originalFallthroughInstruction> 
//      jnc <originalFallthroughInstruction> [UNKNOWNSIGN]
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
// p_addressOriginalInstruction is set when we call method from lea instrumentation
void IntegerTransform::addOverflowCheckForLea(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy, AddressID_t *p_addressOriginalInstruction)
{
	assert(getFileIR() && p_instruction && p_instruction->GetFallthrough());
	
cerr << __func__ << ": comment: " << p_instruction->GetComment() << " annotation: " << p_annotation.toString() << " policy: " << p_policy << endl;

	Instruction_t* jnc_i = NULL;
	string detector(INTEGER_OVERFLOW_DETECTOR);

	// this will be either jno or jnc
	Instruction_t* jncond_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());

	// set fallthrough for the original instruction
	Instruction_t* nextOrig_i = p_instruction->GetFallthrough();
	p_instruction->SetFallthrough(jncond_i); 

	// jncond 
	int isMultiply = isMultiplyInstruction(p_instruction);
	if (isMultiply)
	{
		// fallthrough will be set by the callback handler
		addJno(jncond_i, NULL, nextOrig_i);
		if (p_annotation.getBitWidth() == 32)
			detector = string(MUL_OVERFLOW_DETECTOR_32);
		else if (p_annotation.getBitWidth() == 16)
			detector = string(MUL_OVERFLOW_DETECTOR_16);
		else if (p_annotation.getBitWidth() == 8)
			detector = string(MUL_OVERFLOW_DETECTOR_8);
	}
	else if (p_annotation.isSigned())
	{
		// fallthrough will be set by the callback handler
		addJno(jncond_i, NULL, nextOrig_i);
		if (p_annotation.getBitWidth() == 32)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_UNSIGNED_32);
		else if (p_annotation.getBitWidth() == 16)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_UNSIGNED_16);
		else if (p_annotation.getBitWidth() == 8)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_UNSIGNED_8);
	}
	else if (p_annotation.isUnsigned())
	{
		// fallthrough will be set by the callback handler
		addJnc(jncond_i, NULL, nextOrig_i);
		if (p_annotation.getBitWidth() == 32)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_UNSIGNED_32);
		else if (p_annotation.getBitWidth() == 16)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_UNSIGNED_16);
		else if (p_annotation.getBitWidth() == 8)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_UNSIGNED_8);
	}
	else
	{
		if (p_annotation.getBitWidth() == 32)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_UNKNOWN_32);
		else if (p_annotation.getBitWidth() == 16)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_UNKNOWN_16);
		else if (p_annotation.getBitWidth() == 8)
			detector = string(ADDSUB_OVERFLOW_DETECTOR_UNKNOWN_8);

		jnc_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());

		addJno(jncond_i, jnc_i, nextOrig_i);
		addJnc(jnc_i, NULL, nextOrig_i); // fallthrough will be set by the callback handler
	}

	logMessage(__func__, "detector: " + detector);

	if (jnc_i) // unknown add/sub
	{
		logMessage(__func__, "add callback handler for unknown add/sub");
		addCallbackHandler(detector, p_instruction, jnc_i, nextOrig_i, p_policy, p_addressOriginalInstruction);
	}
	else
	{
		// mul, or signed/unsigned add/sub
		logMessage(__func__, "add callback handler for mul, or signed/unsigned add/sub");
		addCallbackHandler(detector, p_instruction, jncond_i, nextOrig_i, p_policy, p_addressOriginalInstruction);
	}
}

void IntegerTransform::logMessage(const std::string &p_method, const std::string &p_msg)
{
	std::cerr << p_method << ": " << p_msg << std::endl;
}

void IntegerTransform::logMessage(const std::string &p_method, const MEDS_InstructionCheckAnnotation& p_annotation, const std::string &p_msg)
{
	logMessage(p_method, p_msg + " annotation: " + p_annotation.toString());
}
