/*
 * Copyright (c) 2013, 2014 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

#include <assert.h>
#include <sstream>
#include "leapattern.hpp"
#include "integertransform64.hpp"
#include "Rewrite_Utility.hpp"

#define INSTRUMENT_LEA
#define INSTRUMENT_OVERFLOW
#define INSTRUMENT_UNDERFLOW
#define INSTRUMENT_TRUNCATION
#define INSTRUMENT_SIGNEDNESS


using namespace libTransform;

/**
*     64 bit implementation status of the integer transform
*     20140228 64-bit overflows on multiply, signed/unsigned add/sub
*     20140422 callback handlers added
*
*      64-bit        signed     unsigned   unknown
*      --------      ------     --------   -------
*      add, sub       of          c         of,c
*        mul          of          of         of
*        lea         @todo      @todo       @todo
*             
*      32-bit        signed     unsigned   unknown
*      --------      ------     --------   -------
*      add, sub      @todo      @todo       @todo
*        mul          of          of         of
*        lea         @todo      @todo       @todo

*     @todo:
*           - test unknown
*           - test warnings only mode
*           - handle LEA instructions
*
**/

IntegerTransform64::IntegerTransform64(VariantID_t *p_variantID, FileIR_t *p_fileIR, MEDS_Annotations_t *p_annotations, set<std::string> *p_filteredFunctions, set<VirtualOffset> *p_benignFalsePositives) : IntegerTransform(p_variantID, p_fileIR, p_annotations, p_filteredFunctions, p_benignFalsePositives)
{
}

// iterate through all functions
// filter those functions that should be ignored
//    iterate through all instructions in function
//    if MEDS annotation says to instrument
//       add instrumentation
int IntegerTransform64::execute()
{
	if (isWarningsOnly())
		logMessage(__func__, "warnings only mode");
	if (isInstrumentIdioms())
		logMessage(__func__, "override annotations -- instrument IDIOMS");

	for(
	  set<Function_t*>::const_iterator itf=getFileIR()->GetFunctions().begin();
	  itf!=getFileIR()->GetFunctions().end();
	  ++itf
	  )
	{
		Function_t* func=*itf;

		if (getFilteredFunctions()->find(func->GetName()) != getFilteredFunctions()->end())
		{
			logMessage(__func__, "filter out: " + func->GetName());
			continue;
		}

		if (isBlacklisted(func))
		{
			logMessage(__func__, "blacklisted: " + func->GetName());
			m_numBlacklisted++;
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
				int policy = POLICY_DEFAULT; //  default for now is exit -- no callback handlers yet

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

				virtual_offset_t irdb_vo = insn->GetAddress()->GetVirtualOffset();
				if (irdb_vo == 0) continue;

				VirtualOffset vo(irdb_vo);

				if (getAnnotations()->count(vo) == 0)
					continue;
				std::pair<MEDS_Annotations_t::iterator,MEDS_Annotations_t::iterator> ret;
				ret = getAnnotations()->equal_range(vo);
				MEDS_InstructionCheckAnnotation annotation; 
				MEDS_InstructionCheckAnnotation* p_annotation; 
				for ( MEDS_Annotations_t::iterator it = ret.first; it != ret.second; ++it)
				{
					MEDS_AnnotationBase *base_type=(it->second);
					p_annotation = dynamic_cast<MEDS_InstructionCheckAnnotation*>(base_type);
					if( p_annotation == NULL)
						continue;
					annotation = *p_annotation;
					if (!annotation.isValid()) 
						continue;
					else
						break; // let's just handle one annotation for now and see how it goes
				}

				logMessage(__func__, annotation, "-- instruction: " + insn->getDisassembly());
				m_numAnnotations++;

				if (annotation.isIdiom() && !isInstrumentIdioms())
				{
					logMessage(__func__, "skip IDIOM");
					m_numIdioms++;
					continue;
				}

				if (!insn->GetFallthrough())
				{
					logMessage(__func__, "Warning: no fall through for instruction -- skipping");
					continue;
				}

				if (annotation.isOverflow())
				{
#ifdef INSTRUMENT_OVERFLOW
					m_numTotalOverflows++;
					handleOverflowCheck(insn, annotation, policy);
#endif
				}
#ifdef INSTRUMENT_UNDERFLOW
				else 
				if (annotation.isUnderflow())
				{
					m_numTotalUnderflows++;
					handleUnderflowCheck(insn, annotation, policy);
				}
#endif
#ifdef INSTRUMENT_TRUNCATION
				else if (annotation.isTruncation())
				{
					m_numTotalTruncations++;
					handleTruncation(insn, annotation, policy);
				}
#endif
#ifdef INSTRUMENT_SIGNEDNESS
				else if (annotation.isSignedness())
				{
					m_numTotalSignedness++;
					handleSignedness(insn, annotation, policy);
				}
#endif
			}
		} // end iterate over all instructions in a function
	} // end iterate over all functions

	return 0;
}

void IntegerTransform64::handleOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	bool result = false;

	if (isMultiplyInstruction(p_instruction) || (p_annotation.isOverflow() && !p_annotation.isNoFlag()))
	{
		// handle signed/unsigned add/sub overflows (non lea)
		result = addOverflowUnderflowCheck(p_instruction, p_annotation, p_policy);
	}
#ifdef INSTRUMENT_LEA
	else 
	if (p_annotation.isNoFlag())
	{
		// handle lea
		if ((p_annotation.getBitWidth() == 64 || p_annotation.getBitWidth() == 32))
		{
			result = addOverflowCheckNoFlag(p_instruction, p_annotation, p_policy);
		}
	}
#endif

	if (result)
	{
		m_numOverflows++;
	}
	else
	{
		m_numOverflowsSkipped++;
		logMessage(__func__, "OVERFLOW type not yet handled");
	}
}

void IntegerTransform64::handleUnderflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	bool success = false;

	if (p_annotation.isUnderflow() && !p_annotation.isNoFlag())
		success = addOverflowUnderflowCheck(p_instruction, p_annotation, p_policy);

	if (success)
	{
		m_numUnderflows++;
	}
	else
	{
		m_numUnderflowsSkipped++;
		logMessage(__func__, "UNDERFLOW type not yet handled");
	}
}

void IntegerTransform64::handleTruncation(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	bool success = false;

	if (!isMovInstruction(p_instruction))
	{
		logMessage(__func__, "We only instrument MOV instructions for TRUNCATION annotations");
		return;
	}

	if (p_annotation.getTruncationFromWidth() == 64)
	{
		if (p_annotation.getTruncationToWidth() == 32 || p_annotation.getTruncationToWidth() == 16 || p_annotation.getTruncationToWidth() == 8)
		{
			success = addTruncationCheck64(p_instruction, p_annotation, p_policy);
		}
		else
		{
			logMessage(__func__, "Truncation type not yet handled (64)");
		}
	}
	else if (p_annotation.getTruncationFromWidth() == 32)
	{
		if (p_annotation.getTruncationToWidth() == 16 || p_annotation.getTruncationToWidth() == 8)
		{
			success = addTruncationCheck32(p_instruction, p_annotation, p_policy);
		}
		else
		{
			logMessage(__func__, "Truncation type not yet handled (32)");
		}
	}

	if (success)
		m_numTruncations++;
	else
	{
		m_numTruncationsSkipped++;
		logMessage(__func__, "Truncation type not yet handled (2)");
	}
}

void IntegerTransform64::handleSignedness(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	if (addSignednessCheck(p_instruction, p_annotation, p_policy))
	{
                m_numSignedness++;
	}
	else
	{
                logMessage(__func__, "SIGNEDNESS: error: skip instrumentation");
                m_numSignednessSkipped++;
	}
}


//
// Saturation Policy
//	        mul a, b                 ; <instruction to instrument>
//              jno <OrigNext>           ; if no overflows, jump to original fallthrough instruction
//              mov a, MIN/MAX           ; policy = min-saturate (underflow) / max-saturate (overflow)
//              of64/uf64 handler        ; call the callback handler (handle diagnostics)
// OrigNext:    <nextInstruction>        ; original fallthrugh
//
bool IntegerTransform64::addOverflowUnderflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	char tmpbuf[1024];

	assert(getFileIR() && p_instruction && p_instruction->GetFallthrough());
	Register::RegisterName targetReg = getTargetRegister(p_instruction);
	if (targetReg == Register::UNKNOWN)
	{
		logMessage(__func__, p_annotation, "unknown target register");
		return false;
	}
	else if (targetReg == Register::RSP || targetReg == Register::RBP ||
		targetReg == Register::ESP || targetReg == Register::EBP)
	{
		logMessage(__func__, p_annotation, "target register is ESP/RSP or EBP/RBP: skip");
		return false;
	}

	Instruction_t* jncond_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());
	Instruction_t* nop_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());
	Instruction_t* policy_i;

	Instruction_t* next_i = p_instruction->GetFallthrough();
	p_instruction->SetFallthrough(jncond_i); 

	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
		policy_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());
	else
		policy_i = nop_i;

	if (p_annotation.isSigned() || isMultiplyInstruction(p_instruction))
	{
		addJno(jncond_i, policy_i, next_i); 
	}
	else if (p_annotation.isUnsigned())
	{
		addJnc(jncond_i, policy_i, next_i);
	}
	else
	{ 	// unknown sign
		Instruction_t* jnc_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());
		addJno(jncond_i, jnc_i, next_i); 
		addJnc(jnc_i, policy_i, next_i); 
	}

	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
	{
		// @todo:
		//    saturating signed multiply overflow should take into account
		//    the signs of the operand. If multiplying negative by positive #,
		//    then we want to sature to MIN_SIGNED_INT
		
		if (p_annotation.isUnderflow())
               		addMinSaturation(policy_i, targetReg, p_annotation, nop_i); 
		else
       	         	addMaxSaturation(policy_i, targetReg, p_annotation, nop_i);
	}

    std::string detector = p_annotation.isOverflow() ? OVERFLOW64_DETECTOR : UNDERFLOW64_DETECTOR;

	setAssembly(nop_i, "nop");  
    Instruction_t* cb = addCallbackHandlerSequence(p_instruction, next_i, detector, p_policy);
	nop_i->SetFallthrough(cb);

	return true;
}

bool IntegerTransform64::addOverflowCheckNoFlag(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	LEAPattern leaPattern(p_annotation);

	if (!leaPattern.isValid())
	{
		logMessage(__func__, "invalid or unhandled lea pattern - skipping: ");
		return false;
	}

	if (leaPattern.getRegister1() == Register::UNKNOWN ||
		leaPattern.getRegister1() == Register::RSP || leaPattern.getRegister1() == Register::RBP ||
		leaPattern.getRegister1() == Register::ESP || leaPattern.getRegister1() == Register::EBP)
	{
		logMessage(__func__, "destination register is unknown, r/esp or r/ebp -- skipping: ");
		return false;
	}

	if (leaPattern.isRegisterPlusRegister() || leaPattern.isRegisterTimesRegister())
	{
		Register::RegisterName reg1 = leaPattern.getRegister1();
		Register::RegisterName reg2 = leaPattern.getRegister2();
		Register::RegisterName target = getTargetRegister(p_instruction);

		if (reg1 == Register::UNKNOWN || reg2 == Register::UNKNOWN || target == Register::UNKNOWN)
		{
			logMessage(__func__, "lea reg reg pattern: error retrieving register: reg1: " + Register::toString(reg1) + " reg2: " + Register::toString(reg2) + " target: " + Register::toString(target));
			return false;
		}
		else if (reg2 == Register::RSP || target == Register::RBP ||
	           reg2 == Register::ESP || target == Register::EBP) 
		{
			logMessage(__func__, "source or target register is esp/rsp/ebp/rbp -- skipping: ");
			return false;
		}
		else
		{
			if (leaPattern.isRegisterPlusRegister())
				addOverflowCheckNoFlag_RegPlusReg(p_instruction, p_annotation, reg1, reg2, target, p_policy);
			else if (leaPattern.isRegisterTimesRegister())
				addOverflowCheckNoFlag_RegTimesReg(p_instruction, p_annotation, reg1, reg2, target, p_policy);
		}
		return true;
	}
	else if (leaPattern.isRegisterPlusConstant() || leaPattern.isRegisterTimesConstant())
	{
		Register::RegisterName reg1 = leaPattern.getRegister1();
		int k = leaPattern.getConstant();
		Register::RegisterName target = getTargetRegister(p_instruction);

		if (reg1 == Register::UNKNOWN || target == Register::UNKNOWN)
		{
			logMessage(__func__, "lea reg reg pattern: error retrieving register: reg1: " + Register::toString(reg1) + " target: " + Register::toString(target));
			return false;
		}
		else if (target == Register::RSP || target == Register::ESP || target == Register::EBP || target == Register::RBP) 
		{
			logMessage(__func__, "target register is esp/rsp/ebp/rbp -- skipping: ");
			return false;
		}
		else
		{
			if (leaPattern.isRegisterPlusConstant())
				addOverflowCheckNoFlag_RegPlusConstant(p_instruction, p_annotation, reg1, k, target, p_policy);
			else if (leaPattern.isRegisterTimesConstant())
				addOverflowCheckNoFlag_RegTimesConstant(p_instruction, p_annotation, reg1, k, target, p_policy);
		}
		return true;
	}
	logMessage(__func__, "not yet handling lea -- placeholder");
	return false;
}

//
// p_orig          original instruction being instrumented
// p_fallthrough   fallthrough once we're done with the callback handlers/detectors
// p_detector      callback handler function to call
// p_policy        instrumentation policy (e.g.: terminate, saturate, ...)
// 
// returns first instruction of callback handler sequence
//
Instruction_t* IntegerTransform64::addCallbackHandlerSequence(Instruction_t *p_orig, Instruction_t *p_fallthrough, std::string p_detector, int p_policy)
{
	char tmpbuf[1024];

	Instruction_t* lea = addNewAssembly("lea rsp, [rsp-128]");  // red zone 
	lea->SetComment("callback: " + p_detector);

    // must save flags as strata will perturb them
    Instruction_t* saveFlags = addNewAssembly(lea, "pushf");

	// pass in PC of instrumented instruction
	// pass in p_policy 
	sprintf(tmpbuf,"push 0x%08x", p_policy);  
	Instruction_t* instr = addNewAssembly(saveFlags, tmpbuf);
	sprintf(tmpbuf,"push 0x%08x", p_orig->GetAddress()->GetVirtualOffset()); 
	instr = addNewAssembly(instr, tmpbuf);

	Instruction_t* call = allocateNewInstruction(p_orig->GetAddress()->GetFileID(), p_orig->GetFunction());
	instr->SetFallthrough(call);
	setAssembly(call, "call 0"); 
		
	addCallbackHandler64(call, p_detector, 2); // 2 args for now

	assert(call->GetTarget());

	instr = addNewAssembly(call, "lea rsp, [rsp+16]");  
	instr = addNewAssembly(instr, "popf");  
	instr = addNewAssembly(instr, "lea rsp, [rsp+128]");  
	instr->SetFallthrough(p_fallthrough);

	return lea;
}

// Example annotation to handle
// 804852e      3 INSTR CHECK OVERFLOW NOFLAGSIGNED 32 EDX+EAX ZZ lea     eax, [edx+eax] Reg1: EDX Reg2: EAX
// Need to handle both 32-bit and 64-bit versions
//
// Original:
//   lea r3, [r1+r2]
//   <originalNext>
//
// Instrumentation:
//   push r1                ;   save r1
//   pushf                  ;   save flags
//   add r1, r2             ;   r1 = r1 + r2
//        <overflowcheck>   ;   check for overflow 
//          (jno|jnc <restore>)   ; SIGNED|UNSIGNED
//            fallthrough--><policy>
//          (jno&jnc <restore>)   ; UNKNOWNSIGN check both flags  
//            fallthrough--><policy>
//
// <restore>
//         popf                   ; restore flags
//         pop r1                 ; restore register
//
// <orig>: lea r3, [r1+r2]        ; original instruction        
//         <originalNext>         ; original next instruction
//
// <policy>                  
//         () callback handler 
//         popf                   ; restore flags
//         pop r1                 ; restore register
//            fallthrough-->originalNext (if no saturation)
//         saturateMax(r3)        ; optional saturation
//            fallthrough-->originalNext
//
void IntegerTransform64::addOverflowCheckNoFlag_RegPlusReg(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const Register::RegisterName& p_reg2, const Register::RegisterName& p_reg3, int p_policy)
{
	assert(p_instruction && p_instruction->GetFallthrough());

	cerr << __func__ << ": r3 <-- r1+r2: r1: " << Register::toString(p_reg1) << " r2: " << Register::toString(p_reg2) << " target register: " << Register::toString(p_reg3) << "  annotation: " << p_annotation.toString() << endl;

	Instruction_t *origFallthrough = p_instruction->GetFallthrough();
	Instruction_t *instr, *first, *saturation_policy;
	Instruction_t *restore = addNewAssembly("popf");

	saturation_policy = addNewAssembly("nop");
	saturation_policy->SetComment("lea overflow instrumentation(reg+reg): policy code sequence");

	// Original code sequence:
	//   lea r3, [r1+r2]
	//   <originalNext>
	//
	// Instrumentation:
	//   push r1                ;   save r1
	//   pushf                  ;   save flags
	//   add r1, r2             ;   r1 = r1 + r2
	//        <overflowcheck>   ;   check for overflow 
	//          (jno|jnc <restore>)   ; SIGNED|UNSIGNED
	//            fallthrough--><policy>
	//          (jno&jnc <restore>)   ; UNKNOWNSIGN check both flags  
	//            fallthrough--><policy>
	//
//	first = addNewAssembly("push " + Register::toString(p_reg1)); 
//	first->SetComment("lea overflow instrumentation(reg+reg): start");
//	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, first);

	Instruction_t* originalInstrumentInstr = IRDBUtility::insertAssemblyBefore(getFileIR(), p_instruction, "lea rsp, [rsp-128]" , NULL);
	p_instruction->SetComment("lea overflow instrumentation(reg+reg): start");
	instr = addNewAssembly(p_instruction, std::string("push ") + Register::toString(p_reg1));
	instr = addNewAssembly(instr, "pushf");

	instr = addNewAssembly(instr, "add " + Register::toString(p_reg1) + "," + Register::toString(p_reg2));
	if (p_annotation.isSigned())
	{
		instr = addNewAssembly(instr, "jno 0x22");
		instr->SetFallthrough(saturation_policy);
		instr->SetTarget(restore);
		instr->SetComment("signed: lea: reg+reg");
	}
	else if (p_annotation.isUnsigned())
	{
		instr = addNewAssembly(instr, "jnc 0x22");
		instr->SetFallthrough(saturation_policy);
		instr->SetTarget(restore);
	}
	else
	{
		Instruction_t *instr2 = addNewAssembly(instr, "jno 0x22"); // this generates bogus assembly code, why?
		instr2->SetTarget(restore);
		instr = addNewAssembly(instr2, "jnc 0x22");
		instr->SetFallthrough(saturation_policy);
		instr->SetTarget(restore);
	}

	// <policy>                  
	//         nop                    ;
	//         () callback handler    ;
	//         popf                   ; restore flags
	//         pop r1                 ; restore register
	//            fallthrough-->originalNext (if no saturation)
	//         saturateMax(r3)        ; optional saturation
	//            fallthrough-->originalNext
	//
	// assume 64 bit for now
	//
	Instruction_t *popf = addNewAssembly("popf");
	Instruction_t *callback = addCallbackHandlerSequence(p_instruction, popf, OVERFLOW64_DETECTOR, p_policy);
	saturation_policy->SetFallthrough(callback);
	instr = addNewAssembly(popf, "pop " + Register::toString(p_reg1));
	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
	{
		instr = addNewMaxSaturation(instr, p_reg3, p_annotation);
		instr = addNewAssembly(instr, "lea rsp, [rsp+128]");
		instr->SetFallthrough(origFallthrough);
	}
	else
	{
		instr = addNewAssembly(instr, "lea rsp, [rsp+128]");
		instr->SetFallthrough(originalInstrumentInstr);
	}

	// <restore>
	//         popf                   ; restore flags
	//         pop r1                 ; restore register
	//
	// <orig>: lea r3, [r1+r2]        ; original instruction        
	//         <originalNext>         ; original next instruction
	//
	restore->SetComment("lea overflow instrumentation(reg+reg): restore");
	instr = addNewAssembly(restore, "pop " + Register::toString(p_reg1));
	instr = addNewAssembly(instr, "lea rsp, [rsp+128]");
	instr->SetFallthrough(originalInstrumentInstr);
}

// Example annotation to handle
// 804852e      3 INSTR CHECK OVERFLOW NOFLAGSIGNED 32 EDX+EAX ZZ lea     eax, [edx+eax] Reg1: EDX Reg2: EAX
// Need to handle both 32-bit and 64-bit versions
//
// Original:
//   lea r3, [r1+r2]
//   <originalNext>
//
// Instrumentation:
//   push r1                ;   save r1
//   pushf                  ;   save flags
//   imul r1, r2             ;   r1 = r1 * r2
//        <overflowcheck>   ;   check for overflow 
//          (jno|jnc <restore>)   ; SIGNED|UNSIGNED
//            fallthrough--><policy>
//          (jno&jnc <restore>)   ; UNKNOWNSIGN check both flags  
//            fallthrough--><policy>
//
// <restore>
//         popf                   ; restore flags
//         pop r1                 ; restore register
//
// <orig>: lea r3, [r1+r2]        ; original instruction        
//         <originalNext>         ; original next instruction
//
// <policy>                  
//         () callback handler 
//         popf                   ; restore flags
//         pop r1                 ; restore register
//            fallthrough-->originalNext (if no saturation)
//         saturateMax(r3)        ; optional saturation
//            fallthrough-->originalNext
//
void IntegerTransform64::addOverflowCheckNoFlag_RegTimesReg(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const Register::RegisterName& p_reg2, const Register::RegisterName& p_reg3, int p_policy)
{
	assert(p_instruction && p_instruction->GetFallthrough());

	cerr << __func__ << ": r3 <-- r1*r2: r1: " << Register::toString(p_reg1) << " r2: " << Register::toString(p_reg2) << " target register: " << Register::toString(p_reg3) << "  annotation: " << p_annotation.toString() << endl;

	Instruction_t *origFallthrough = p_instruction->GetFallthrough();
	Instruction_t *instr, *first, *saturation_policy;
	Instruction_t *restore = addNewAssembly("popf");
	saturation_policy = addNewAssembly("nop");
	saturation_policy->SetComment("lea overflow instrumentation(reg*reg): policy code sequence");


	// Original code sequence:
	//   lea r3, [r1*r2]
	//   <originalNext>
	//
	// Instrumentation:
	//   push r1                ;   save r1
	//   pushf                  ;   save flags
	//   imul r1, r2            ;   r1 = r1 * r2
	//        <overflowcheck>   ;   check for overflow 
	//        jno <restore>     ; 
	//           fallthrough--><policy>
	//
	Instruction_t* originalInstrumentInstr = IRDBUtility::insertAssemblyBefore(getFileIR(), p_instruction, "lea rsp, [rsp-128]" , NULL);
	p_instruction->SetComment("lea overflow instrumentation(reg*reg): start");
	instr = addNewAssembly(p_instruction, std::string("push ") + Register::toString(p_reg1));
	instr = addNewAssembly(instr, "pushf");

	instr = addNewAssembly(instr, "imul " + Register::toString(p_reg1) + "," + Register::toString(p_reg2));
	instr = addNewAssembly(instr, "jno 0x22");
	instr->SetFallthrough(saturation_policy);
	instr->SetTarget(restore);

	// <policy>                  
	//         nop                    ;
	//         () callback handler    ;
	//         popf                   ; restore flags
	//         pop r1                 ; restore register
	//            fallthrough-->originalNext (if no saturation)
	//         saturateMax(r3)        ; optional saturation
	//            fallthrough-->originalNext
	//
	// assume 64 bit for now
	//
	Instruction_t *popf = addNewAssembly("popf");
	Instruction_t *callback = addCallbackHandlerSequence(p_instruction, popf, OVERFLOW64_DETECTOR, p_policy);
	saturation_policy->SetFallthrough(callback);
	instr = addNewAssembly(popf, "pop " + Register::toString(p_reg1));
	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
	{
		instr = addNewMaxSaturation(instr, p_reg3, p_annotation);
		instr = addNewAssembly(instr, "lea rsp, [rsp+128]");
		instr->SetFallthrough(origFallthrough);
	}
	else
	{
		instr = addNewAssembly(instr, "lea rsp, [rsp+128]");
		instr->SetFallthrough(originalInstrumentInstr);
	}

	// <restore>
	//         popf                   ; restore flags
	//         pop r1                 ; restore register
	//
	// <orig>: lea r3, [r1*r2]        ; original instruction        
	//         <originalNext>         ; original next instruction
	//
	restore->SetComment("lea overflow instrumentation(reg*reg): restore");
	instr = addNewAssembly(restore, "pop " + Register::toString(p_reg1));
	instr = addNewAssembly(instr, "lea rsp, [rsp+128]");
	instr->SetFallthrough(originalInstrumentInstr);
}

// Example annotation to handle
// 804852e      3 INSTR CHECK OVERFLOW NOFLAGSIGNED 32 EDX+EAX ZZ lea     eax, [edx+k] Reg1: EDX Reg3: EAX
// Need to handle both 32-bit and 64-bit versions
//
// Original:
//   lea r3, [r1+k]
//   <originalNext>
//
// Instrumentation:
//   push r1                ;   save r1
//   pushf                  ;   save flags
//   add r1, k              ;   r1 = r1 + k
//        <overflowcheck>   ;   check for overflow 
//          (jno|jnc <restore>)   ; SIGNED|UNSIGNED
//            fallthrough--><policy>
//          (jno&jnc <restore>)   ; UNKNOWNSIGN check both flags  
//            fallthrough--><policy>
//
// <restore>
//         popf                   ; restore flags
//         pop r1                 ; restore register
//
// <orig>: lea r3, [r1+k]         ; original instruction        
//         <originalNext>         ; original next instruction
//
// <policy>                  
//         () callback handler 
//         popf                   ; restore flags
//         pop r1                 ; restore register
//            fallthrough-->originalNext (if no saturation)
//         saturateMax(r3)        ; optional saturation
//            fallthrough-->originalNext
//
void IntegerTransform64::addOverflowCheckNoFlag_RegPlusConstant(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const int p_constant, const Register::RegisterName& p_reg3, int p_policy)
{
	assert(p_instruction && p_instruction->GetFallthrough());

	if (p_annotation.isUnsigned() && (p_constant < 0)) {
	  logMessage(__func__, "lea reg+neg constant pattern: skip this annotation type (prone to false positives)");
	  m_numOverflowsSkipped++;
	  return;
	}

	cerr << __func__ << ": r3 <-- r1+k: r1: " << Register::toString(p_reg1) << " k: " << p_constant << " target register: " << Register::toString(p_reg3) << "  annotation: " << p_annotation.toString() << endl;

	Instruction_t *origFallthrough = p_instruction->GetFallthrough();
	Instruction_t *instr, *first, *saturation_policy;
	Instruction_t *restore = addNewAssembly("popf");
	saturation_policy = addNewAssembly("nop");
	saturation_policy->SetComment("lea overflow instrumentation(reg+k): policy code sequence");

	// Original code sequence:
	//   lea r3, [r1+k]
	//   <originalNext>
	//
	// Instrumentation:
	//   push r1                ;   save r1
	//   pushf                  ;   save flags
	//   add r1, k              ;   r1 = r1 + k
	// (or sub r1,-k            ;   r1 = r1 - (-k) for k < 0)
	//        <overflowcheck>   ;   check for overflow 
	//          (jno|jnc <restore>)   ; SIGNED|UNSIGNED
	//            fallthrough--><policy>
	//          (jno&jnc <restore>)   ; UNKNOWNSIGN check both flags  
	//            fallthrough--><policy>
	//
/*
	first = addNewAssembly("push " + Register::toString(p_reg1)); 
	first->SetComment("lea overflow instrumentation(reg+k): start");
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, first);
	instr = addNewAssembly(first, "pushf");
	*/

	Instruction_t* originalInstrumentInstr = IRDBUtility::insertAssemblyBefore(getFileIR(), p_instruction, "lea rsp, [rsp-128]" , NULL);
	p_instruction->SetComment("lea overflow instrumentation(reg+k): start");
	instr = addNewAssembly(p_instruction, std::string("push ") + Register::toString(p_reg1));
	instr = addNewAssembly(instr, "pushf");
//-------------------
	// make sure we set the fallthrough post careful insertion
//	first->SetFallthrough(instr);

	std::ostringstream s;
	if (p_constant >= 0) {
	  s << "add " << Register::toString(p_reg1) << "," << p_constant;
	}
	else {
	  // NOTE: We are short-circuiting this case currently; see top of
	  //  this function. Should never get here.
	  s << "sub " << Register::toString(p_reg1) << "," << (-p_constant);
	}
	instr = addNewAssembly(instr, s.str());
	if (p_annotation.isSigned())
	{
		instr = addNewAssembly(instr, "jno 0x22");
		instr->SetFallthrough(saturation_policy);
		instr->SetTarget(restore);
	}
	else if (p_annotation.isUnsigned())
	{
		instr = addNewAssembly(instr, "jnc 0x22");
		instr->SetFallthrough(saturation_policy);
		instr->SetTarget(restore);
	}
	else
	{
		Instruction_t *instr2 = addNewAssembly(instr, "jno 0x22");
		instr2->SetTarget(restore);
		instr = addNewAssembly(instr2, "jnc 0x22");
		instr->SetFallthrough(saturation_policy);
		instr->SetTarget(restore);
	}

	// <policy>                  
	//         nop                    ;
	//         () callback handler    ;
	//         popf                   ; restore flags
	//         pop r1                 ; restore register
	//            fallthrough-->originalNext (if no saturation)
	//         saturateMax(r3)        ; optional saturation
	//            fallthrough-->originalNext
	//
	// assume 64 bit for now
	//
	Instruction_t *popf = addNewAssembly("popf");
	Instruction_t *callback = addCallbackHandlerSequence(p_instruction, popf, OVERFLOW64_DETECTOR, p_policy);
	saturation_policy->SetFallthrough(callback);
	instr = addNewAssembly(popf, "pop " + Register::toString(p_reg1));
	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
	{
		instr = addNewMaxSaturation(instr, p_reg3, p_annotation);
		instr = addNewAssembly(instr, "lea rsp, [rsp+128]");
		instr->SetFallthrough(origFallthrough);
	}
	else
	{
		instr = addNewAssembly(instr, "lea rsp, [rsp+128]");
		instr->SetFallthrough(originalInstrumentInstr);
	}

	// <restore>
	//         popf                   ; restore flags
	//         pop r1                 ; restore register
	//
	// <orig>: lea r3, [r1+k]         ; original instruction        
	//         <originalNext>         ; original next instruction
	//
	restore->SetComment("lea overflow instrumentation(reg+k): restore");
	instr = addNewAssembly(restore, "pop " + Register::toString(p_reg1));
	instr = addNewAssembly(instr, "lea rsp, [rsp+128]");
	instr->SetFallthrough(originalInstrumentInstr);
	originalInstrumentInstr->SetFallthrough(origFallthrough);
}


// Example annotation to handle
// 804852e      3 INSTR CHECK OVERFLOW NOFLAGSIGNED 32 EDX+EAX ZZ lea     eax, [edx+eax] Reg1: EDX Reg2: EAX
// Need to handle both 32-bit and 64-bit versions
//
// Original:
//   lea r3, [r1*k]
//   <originalNext>
//
// Instrumentation:
//   push r1                ;   save r1
//   pushf                  ;   save flags
//   imul r1, r2            ;   r1 = r1 * r2
//        <overflowcheck>   ;   check for overflow 
//          (jno <restore>) ; 
//            fallthrough--><policy>
//
// <restore>
//         popf                   ; restore flags
//         pop r1                 ; restore register
//
// <orig>: lea r3, [r1*k]        ; original instruction        
//         <originalNext>         ; original next instruction
//
// <policy>                  
//         () callback handler 
//         popf                   ; restore flags
//         pop r1                 ; restore register
//            fallthrough-->originalNext (if no saturation)
//         saturateMax(r3)        ; optional saturation
//            fallthrough-->originalNext
//
void IntegerTransform64::addOverflowCheckNoFlag_RegTimesConstant(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, const Register::RegisterName& p_reg1, const int p_constant, const Register::RegisterName& p_reg3, int p_policy)
{
	assert(p_instruction && p_instruction->GetFallthrough());

	cerr << __func__ << ": r3 <-- r1*k: r1: " << Register::toString(p_reg1) << " k: " << p_constant << " target register: " << Register::toString(p_reg3) << "  annotation: " << p_annotation.toString() << endl;

	Instruction_t *origFallthrough = p_instruction->GetFallthrough();
	Instruction_t *instr, *first, *saturation_policy;
	Instruction_t *restore = addNewAssembly("popf");
	saturation_policy = addNewAssembly("nop");
	saturation_policy->SetComment("lea overflow instrumentation(reg*k): policy code sequence");

	// Original code sequence:
	//   lea r3, [r1*k]
	//   <originalNext>
	//
	// Instrumentation:
	//   push r1                ;   save r1
	//   pushf                  ;   save flags
	//   imul r1, k            ;   r1 = r1 * k
	//        <overflowcheck>   ;   check for overflow 
	//        jno <restore>     ; 
	//           fallthrough--><policy>
	//
	/*
	first = addNewAssembly("push " + Register::toString(p_reg1)); 
	first->SetComment("lea overflow instrumentation(reg*k): start");
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, first);
	instr = addNewAssembly(first, "pushf");
	*/

	Instruction_t* originalInstrumentInstr = IRDBUtility::insertAssemblyBefore(getFileIR(), p_instruction, "lea rsp, [rsp-128]" , NULL);
	p_instruction->SetComment("lea overflow instrumentation(reg*k): start");
	instr = addNewAssembly(p_instruction, std::string("push ") + Register::toString(p_reg1));
	instr = addNewAssembly(instr, "pushf");
/*--------------*/
	// make sure we set the fallthrough post careful insertion
//	first->SetFallthrough(instr);

	std::ostringstream s;
	s << "imul " << Register::toString(p_reg1) << "," << p_constant;
	instr = addNewAssembly(instr, s.str());
	instr = addNewAssembly(instr, "jno 0x22");
	instr->SetFallthrough(saturation_policy);
	instr->SetTarget(restore);

	// <policy>                  
	//         nop                    ;
	//         () callback handler    ;
	//         popf                   ; restore flags
	//         pop r1                 ; restore register
	//            fallthrough-->originalNext (if no saturation)
	//         saturateMax(r3)        ; optional saturation
	//            fallthrough-->originalNext
	//
	// assume 64 bit for now
	//
	Instruction_t *popf = addNewAssembly("popf");
	Instruction_t *callback = addCallbackHandlerSequence(p_instruction, popf, OVERFLOW64_DETECTOR, p_policy);
	saturation_policy->SetFallthrough(callback);
	instr = addNewAssembly(popf, "pop " + Register::toString(p_reg1));
	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
	{
		instr = addNewMaxSaturation(instr, p_reg3, p_annotation);
		instr = addNewAssembly(instr, "lea rsp, [rsp+128]");
		instr->SetFallthrough(origFallthrough);
	}
	else
	{
		instr = addNewAssembly(instr, "lea rsp, [rsp+128]");
		instr->SetFallthrough(originalInstrumentInstr);
	}

	// <restore>
	//         popf                   ; restore flags
	//         pop r1                 ; restore register
	//
	// <orig>: lea r3, [r1*k]        ; original instruction        
	//         <originalNext>         ; original next instruction
	//
	restore->SetComment("lea overflow instrumentation(reg*reg): restore");
	instr = addNewAssembly(restore, "pop " + Register::toString(p_reg1));
	instr = addNewAssembly(instr, "lea rsp, [rsp+128]");
	instr->SetFallthrough(originalInstrumentInstr);
	originalInstrumentInstr->SetFallthrough(origFallthrough);
}

//
// STARS has extensive discussion of the logic behind truncation and
//  signedness annotations in module SMPInstr.cpp, method EmitIntegerErrorAnnotation().
//  The possible combinations are categorized in terms of the instrumentation
//  needed at run time:
//  CHECK TRUNCATION UNSIGNED:    discarded bits must be all zeroes.
//  CHECK TRUNCATION SIGNED:      discarded bits must be sign-extension of stored bits.
//  CHECK TRUNCATION UNKNOWNSIGN: discarded bits must be all zeroes, OR must
//                                  be the sign-extension of the stored bits.
// All truncation annotations are emitted on stores (mov opcodes in x86).
// Other possibilities might be handled in the future.
//

bool IntegerTransform64::addTruncationCheck32(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	assert(p_instruction && p_instruction->GetFallthrough() &&
		p_annotation.getTruncationFromWidth() == 32 && 
		(p_annotation.getTruncationToWidth() == 16 || p_annotation.getTruncationToWidth() == 8) &&
		isMovInstruction(p_instruction));

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

	string detector;
	unsigned mask = 0, mask2 = 0;
	string saturationValue = "0x0";

	p_instruction->SetComment("monitor for truncation32");

	if (p_annotation.getTruncationToWidth() == 16) 
	{
		mask = 0xFFFF0000;
		mask2 = 0xFFFF8000;

		if(p_annotation.flowsIntoCriticalSink())
		{
			detector = TRUNCATION64_FORCE_EXIT;
		}
		else if (p_annotation.isUnsigned())
		{
			saturationValue = "0xFFFF";
			detector = string(TRUNCATION64_DETECTOR_UNSIGNED_32_16);
		}
		else if (p_annotation.isSigned())
		{
			saturationValue = "0x7FFF";
			detector = string(TRUNCATION64_DETECTOR_SIGNED_32_16);
		}
		else
		{
			saturationValue = "0x7FFF";
			detector = string(TRUNCATION64_DETECTOR_UNKNOWN_32_16);
		}
	}
	else if (p_annotation.getTruncationToWidth() == 8) 
	{
		mask = 0xFFFFFF00;
		mask2 = 0xFFFFFF80;
		if(p_annotation.flowsIntoCriticalSink())
		{
			detector = TRUNCATION64_FORCE_EXIT;
		}
		else if (p_annotation.isUnsigned())
		{
			saturationValue = "0xFF";
			detector = string(TRUNCATION64_DETECTOR_UNSIGNED_32_8);
		}
		else if (p_annotation.isSigned())
		{
			saturationValue = "0x7F";
			detector = string(TRUNCATION64_DETECTOR_SIGNED_32_8);
		}
		else
		{
			saturationValue = "0x7F";
			detector = string(TRUNCATION64_DETECTOR_UNKNOWN_32_8);
		}
	}

	//             <save flags>
	//             test eax, 0xFFFFFF00   ; (for 8 bit) 
	//             jz <continue>          ; upper 24 bits are 0's 
	//
	//             (invoke truncation handler)
	//             nop ; truncation handler returns here
	//      SAT:   mov [ebp+var_4], <saturate> ; optional
    //             jmp origFT
	//
	// continue:   <restore flags>
	//             mov [ebp+var_4], al    ; <originalInstruction>
	// origFT:     <originalFallThroughInstruction>
	//

	char buf[MAX_ASSEMBLY_SIZE];
	Instruction_t *origFallthrough = p_instruction->GetFallthrough();
	Instruction_t *instr, *save, *test;
	Instruction_t *restore = addNewAssembly("popf");
	restore->SetComment("trunc: restore flags");

/*
	save = addNewAssembly("pushf");
	save->SetComment("start truncation sequence");
*/

	if (p_annotation.isSigned())
	{
		sprintf(buf, "test %s, 0x%8x", Register::toString(p_annotation.getRegister()).c_str(), mask2);
	}
	else
	{
		sprintf(buf, "test %s, 0x%8x", Register::toString(p_annotation.getRegister()).c_str(), mask);
	}
	logMessage(__func__, buf);

/*
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, save);
	test = addNewAssembly(save, buf);
*/
/*--------------*/
//	Instruction_t* originalInstrumentInstr = IRDBUtility::insertAssemblyBefore(getFileIR(), p_instruction, std::string("pushf"), NULL);
	Instruction_t* originalInstrumentInstr = IRDBUtility::insertAssemblyBefore(getFileIR(), p_instruction, "lea rsp, [rsp-128]" , NULL);
	p_instruction->SetComment("start truncation sequence");
	instr = addNewAssembly(p_instruction, "pushf");
	test = addNewAssembly(instr, buf);
/*--------------*/
 //   p_instruction->SetFallthrough(test);

//	save->SetFallthrough(test);

    instr = addNewAssembly(restore, "lea rsp, [rsp+128]");
	instr->SetFallthrough(originalInstrumentInstr);

	if (p_annotation.isUnsigned())
	{
		logMessage(__func__, "unsigned annotation");
		Instruction_t *nop = addNewAssembly("nop");

		instr = addNewAssembly(test, "jz 0x22"); // bogus jump target for now
		instr->SetTarget(restore);
		instr->SetFallthrough(nop);
	
		Instruction_t *callback; 

		if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
		{
			string saturationInstruction = "mov " + p_annotation.getTarget2() + ", " + saturationValue;
			Instruction_t *saturation = addNewAssembly(saturationInstruction);
			saturation->SetComment("trunc/unsigned/saturate");
			
			callback = addCallbackHandlerSequence(p_instruction, saturation, detector, p_policy);
			nop->SetFallthrough(callback);
			saturation->SetFallthrough(restore);
		}
		else
		{
			callback = addCallbackHandlerSequence(p_instruction, restore, detector, p_policy);
			nop->SetFallthrough(callback);
		}
	}
	else
	{
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
		logMessage(__func__, "signed/unknown annotation");
		Instruction_t *nop = addNewAssembly("nop");

		instr = addNewAssembly(test, "jz 0x22"); // bogus jump target for now
		instr->SetTarget(restore);
		
		std::ostringstream s;
		s << "cmp " << Register::toString(p_annotation.getRegister()) << ", 0x" << hex << mask2 << dec;
		instr = addNewAssembly(instr, s.str());

		instr = addNewAssembly(instr, "jae 0x22"); 
		instr->SetTarget(restore);
		instr->SetFallthrough(nop);

		Instruction_t *callback; 

		if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
		{
			string saturationInstruction = "mov " + p_annotation.getTarget2() + ", " + saturationValue;
			Instruction_t *saturation = addNewAssembly(saturationInstruction);
			if (p_annotation.isSigned())
				saturation->SetComment("trunc/signed/saturate");
			else
				saturation->SetComment("trunc/unknown/saturate");
			
			callback = addCallbackHandlerSequence(p_instruction, saturation, detector, p_policy);
			nop->SetFallthrough(callback);
			saturation->SetFallthrough(restore);
		}
		else
		{
			callback = addCallbackHandlerSequence(p_instruction, restore, detector, p_policy);
			nop->SetFallthrough(callback);
		}
	}

	return true;
}

//    we want:  32->16  movzx edx, ax -->  movzx edx, 0xFFFF
//              32->8   movzx edx, al -->  movzx edx, 0xFF
//
// let's handle 2-operand instructions of the form:
//            mov *, <reg>
//

string IntegerTransform64::buildSaturationAssembly(Instruction_t *p_instruction, string p_pattern, string p_value)
{
	// @todo: verify 2 operands!

	string i = p_instruction->getDisassembly();

	convertToLowercase(i);
	convertToLowercase(p_pattern);
	convertToLowercase(p_value);

	size_t commaPos = i.find_last_of(',');
	if (commaPos != string::npos)
		i.replace(i.find(p_pattern, commaPos), p_pattern.size(), p_value);
	else
		i.replace(i.find(p_pattern), p_pattern.size(), p_value);
	return i;
}

//
// STARS has extensive discussion of the logic behind truncation and
//  signedness annotations in module SMPInstr.cpp, method EmitIntegerErrorAnnotation().
//  The possible combinations are categorized in terms of the instrumentation
//  needed at run time:
//  CHECK TRUNCATION UNSIGNED:    discarded bits must be all zeroes.
//  CHECK TRUNCATION SIGNED:      discarded bits must be sign-extension of stored bits.
//  CHECK TRUNCATION UNKNOWNSIGN: discarded bits must be all zeroes, OR must
//                                  be the sign-extension of the stored bits.
// All truncation annotations are emitted on stores (mov opcodes in x86).
// Other possibilities might be handled in the future.
//

bool IntegerTransform64::addTruncationCheck64(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	assert(p_instruction && p_instruction->GetFallthrough() &&
		p_annotation.getTruncationFromWidth() == 64 && 
		(p_annotation.getTruncationToWidth() == 32 || p_annotation.getTruncationToWidth() == 16 || p_annotation.getTruncationToWidth() == 8) &&
		isMovInstruction(p_instruction));

	std::set<Register::RegisterName> takenRegs;
	takenRegs.insert(p_annotation.getRegister()); 
	takenRegs.insert(p_annotation.getRegister2());
	takenRegs.insert(Register::RSP); // don't mess with the stack pointer
	takenRegs.insert(Register::RBP); // don't mess with the frame pointer
	Register::RegisterName borrowReg = Register::getFreeRegister64(takenRegs);

	if (borrowReg == Register::UNKNOWN)
	{
		logMessage(__func__, "Could not borrow a 64-bit register");
		return false;
	}

	// Main difference between TRUNCATION annotation for 64 bits vs. 32 bits
	// is that we need to use a register as IA X86-64 doesn't support 64 bit constants
	// for most instructions

	// Complicated case:
	// 80484ed 3 INSTR CHECK TRUNCATION UNKNOWNSIGN 32 EAX 8 AL ZZ mov     [ebp+var_4], al
	//  example: for unknownsign truncation - 8 bit on AL
	//          it's ok if 24 upper bits are sign-extension or all 0's
	//          Re-phrased: upper 24 are 0s, upper 25 are 0s, upper 25 are 1s
	//           are all OK in the UNKNOWNSIGN case
	//          Note that upper 24 are 0's means no need to check for the
	//           case where the upper 25 are 0's; already OK.
	//
	//             <save reg>
	//             <save flags>
	//             mov <reg>, 0xFFFFFFFFFFFFFF00  ; // need to grab register 
	//             test eax, <reg>        ; (for 8 bit) 
	//             jz <continue>          ; upper 56 bits are 0's 
	//
	//             cmp eax, 0xFFFFFFFFFFFFFF80    ; (for 8 bit) 
	//             jae continue           ; upper 57 bits are 1's
	//             (invoke truncation handler)
	//             nop ; truncation handler returns here
	//      SAT:   saturating-arithmetic  ; optional
	//
	// continue:   <restore flags>
	//             <restore reg>
	//             mov [ebp+var_4], al    ; <originalInstruction>
	//

	// Unsigned case:
	// 80484ed 3 INSTR CHECK TRUNCATION UNSIGNED 64 RAX 32 EAX ZZ mov     [ebp+var_4], al
	//
	//             <save reg>
	//             <save flags>
	//             mov <reg>, 0xFFFFFFFFFFFFFF00  ; // need to grab register 
	//             test eax, <reg>; (for 64->32 bit) 
	//             jz <continue>          ; upper 32 bits are 0's 
	//
	//             (invoke truncation handler)
	//             nop ; truncation handler returns here
	//      SAT:   saturating-arithmetic  ; optional
	//
	// continue:   <restore flags>
	//             <restore reg>
	//             mov [ebp+var_4], al    ; <originalInstruction>
	//

	string detector;
	long long unsigned mask = 0, mask2 = 0;
	string saturationValue = "0x0";

	p_instruction->SetComment("monitor for truncation64");

	if (p_annotation.getTruncationToWidth() == 32) 
	{
		mask  = 0xFFFFFFFF00000000;
		mask2 = 0xFFFFFFFF80000000;

		if(p_annotation.flowsIntoCriticalSink())
		{
			detector = TRUNCATION64_FORCE_EXIT;
		}
		else if (p_annotation.isUnsigned())
		{
			saturationValue = "0xFFFFFFFF";
			detector = string(TRUNCATION64_DETECTOR_UNSIGNED_64_32);
		}
		else if (p_annotation.isSigned())
		{
			saturationValue = "0x7FFFFFFF";
			detector = string(TRUNCATION64_DETECTOR_SIGNED_64_32);
		}
		else
		{
			saturationValue = "0x7FFFFFFF";
			detector = string(TRUNCATION64_DETECTOR_UNKNOWN_64_32);
		}
	}
	else if (p_annotation.getTruncationToWidth() == 16) 
	{
		mask  = 0xFFFFFFFFFFFF0000;
		mask2 = 0xFFFFFFFFFFFF8000;

		if(p_annotation.flowsIntoCriticalSink())
		{
			detector = TRUNCATION64_FORCE_EXIT;
		}
		else if (p_annotation.isUnsigned())
		{
			saturationValue = "0xFFFF";
			detector = string(TRUNCATION64_DETECTOR_UNSIGNED_64_16);
		}
		else if (p_annotation.isSigned())
		{
			saturationValue = "0x7FFF";
			detector = string(TRUNCATION64_DETECTOR_SIGNED_64_16);
		}
		else
		{
			saturationValue = "0x7FFF";
			detector = string(TRUNCATION64_DETECTOR_UNKNOWN_64_16);
		}
	}
	else if (p_annotation.getTruncationToWidth() == 8) 
	{
		mask  = 0xFFFFFFFFFFFFFF00;
		mask2 = 0xFFFFFFFFFFFFFF80;
		if(p_annotation.flowsIntoCriticalSink())
		{
			detector = TRUNCATION64_FORCE_EXIT;
		}
		else if (p_annotation.isUnsigned())
		{
			saturationValue = "0xFF";
			detector = string(TRUNCATION64_DETECTOR_UNSIGNED_64_8);
		}
		else if (p_annotation.isSigned())
		{
			saturationValue = "0x7F";
			detector = string(TRUNCATION64_DETECTOR_SIGNED_64_8);
		}
		else
		{
			saturationValue = "0x7F";
			detector = string(TRUNCATION64_DETECTOR_UNKNOWN_64_8);
		}
	}

	//             <save reg>
	//             <save flags>
	//             mov <reg>, 0xFFFFFFFFFFFFFF00  ; // need to grab register 
	//             test eax, <reg>        ; (for 8 bit) 
	//             jz <continue>          ; upper 24 bits are 0's 
	//
	//             (invoke truncation handler)
	//             nop ; truncation handler returns here
	//      SAT:   mov [ebp+var_4], <saturate> ; optional
	//             jmp origFT
	//
	// continue:   <restore flags>
	//             <restore reg>
	//             mov [ebp+var_4], al    ; <originalInstruction>
	// origFT:     <originalFallThroughInstruction>
	//

	char buf[MAX_ASSEMBLY_SIZE];
	Instruction_t *origFallthrough = p_instruction->GetFallthrough();
	Instruction_t *instr, *save, *test, *pushf;
	Instruction_t *restore = addNewAssembly("popf");
	restore->SetComment("trunc: restore flags");
	Instruction_t *restoreReg = addNewAssembly("pop " + Register::toString(borrowReg));

/*
	save = addNewAssembly("push " + Register::toString(borrowReg));
	save->SetComment("start truncation sequence");

	pushf = addNewAssembly(save, "pushf");

	if (p_annotation.isSigned())
		sprintf(buf, "mov %s, 0x%16llx", Register::toString(borrowReg).c_str(), mask2);
	else
		sprintf(buf, "mov %s, 0x%16llx", Register::toString(borrowReg).c_str(), mask);

	instr = addNewAssembly(pushf, buf);

	sprintf(buf, "test %s, %s", Register::toString(p_annotation.getRegister()).c_str(), Register::toString(borrowReg).c_str());

*/

/*
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, save);
	test = addNewAssembly(instr, buf);
	*/
/*--------------*/
//	Instruction_t* originalInstrumentInstr = IRDBUtility::insertAssemblyBefore(getFileIR(), p_instruction, std::string("push ") + Register::toString(borrowReg), NULL);
//	Instruction_t* lea = addNewAssembly("lea rsp, [rsp-128]");  // red zone 
	Instruction_t* originalInstrumentInstr = IRDBUtility::insertAssemblyBefore(getFileIR(), p_instruction, "lea rsp, [rsp-128]" , NULL);
	p_instruction->SetComment("start truncation sequence");
	instr = addNewAssembly(p_instruction, std::string("push ") + Register::toString(borrowReg));
	instr = addNewAssembly(instr, "pushf");
	if (p_annotation.isSigned())
		sprintf(buf, "mov %s, 0x%16llx", Register::toString(borrowReg).c_str(), mask2);
	else
		sprintf(buf, "mov %s, 0x%16llx", Register::toString(borrowReg).c_str(), mask);
	instr = addNewAssembly(instr, buf);
	sprintf(buf, "test %s, %s", Register::toString(p_annotation.getRegister()).c_str(), Register::toString(borrowReg).c_str());
	test = addNewAssembly(instr, buf);
	//             <save reg>
	//             <save flags>
	//             mov <reg>, 0xFFFFFFFFFFFFFF00  ; // need to grab register 
	//             test eax, <reg>        ; (for 8 bit) 
/*--------------*/

//	save->SetFallthrough(pushf);

    restore->SetFallthrough(restoreReg);
	instr = addNewAssembly(restoreReg, "lea rsp, [rsp+128]");
	instr->SetFallthrough(originalInstrumentInstr);

	if (p_annotation.isUnsigned())
	{
		logMessage(__func__, "unsigned annotation");
		Instruction_t *nop = addNewAssembly("nop");

		instr = addNewAssembly(test, "jz 0x22"); // bogus jump target for now
		instr->SetTarget(restore);
		instr->SetFallthrough(nop);
	
		Instruction_t *callback; 

		if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
		{
			string saturationInstruction = "mov " + p_annotation.getTarget2() + ", " + saturationValue;
			Instruction_t *saturation = addNewAssembly(saturationInstruction);
			saturation->SetComment("trunc/unsigned/saturate");
			
			callback = addCallbackHandlerSequence(p_instruction, saturation, detector, p_policy);
			nop->SetFallthrough(callback);
			saturation->SetFallthrough(restore);
		}
		else
		{
			callback = addCallbackHandlerSequence(p_instruction, restore, detector, p_policy);
			nop->SetFallthrough(callback);
		}
	}
	else
	{
		// Signed and unknown signed cases are almost identical:
		// 80484ed 3 INSTR CHECK TRUNCATION SIGNED 32 EAX 8 AL ZZ mov     [ebp+var_4], al
		// 80484ed 3 INSTR CHECK TRUNCATION UNKNOWNSIGN 32 EAX 8 AL ZZ mov     [ebp+var_4], al
		//  example: for signed truncation - 8 bit on AL
		//          it's ok if 25 upper bits are all 1's or all 0's
		//  example: for unknownsign truncation - 8 bit on AL
		//          it's ok if 25 upper bits are all 1's or 24 upper bits all 0's
		//
		//             <save reg>
		//             <save flags>
		//             mov <reg>, 0xFFFFFFFFFFFFFF80   
		//             test eax, <reg> ; (for 8 bit) 0xFFFFFFFFFFFFFF00 for UNKNOWN
		//             jz <continue>          ; upper 57 bits are 0's 
		//
		//             cmp eax,  0xFFFFFFFFFFFFFF80    ;(for 8 bit) 
		//             jae continue           ; upper 57 bits are 1's
		//             (invoke truncation handler) 
		//             nop ; truncation handler callback here
		//      SAT:   saturating-arithmetic  ; optional
		//
		// continue:   <restore flags>
		//             <restore reg>
		//             mov [ebp+var_4], al    ; <originalInstruction>
		//
		logMessage(__func__, "signed/unknown annotation");
		Instruction_t *nop = addNewAssembly("nop");

		instr = addNewAssembly(test, "jz 0x22"); // bogus jump target for now
		instr->SetTarget(restore);
		
		std::ostringstream s;
		s << "mov " << Register::toString(borrowReg) << ", 0x" << hex << mask2 << dec;
		instr = addNewAssembly(instr, s.str());

		std::ostringstream s2;
		s2 << "cmp " << Register::toString(p_annotation.getRegister()) << "," << Register::toString(borrowReg);
		instr = addNewAssembly(instr, s2.str());

		instr = addNewAssembly(instr, "jae 0x22"); 
		instr->SetTarget(restore);
		instr->SetFallthrough(nop);

		Instruction_t *callback; 

		if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
		{
			string saturationInstruction = "mov " + p_annotation.getTarget2() + ", " + saturationValue;
			Instruction_t *saturation = addNewAssembly(saturationInstruction);
			if (p_annotation.isSigned())
				saturation->SetComment("trunc/signed/saturate");
			else
				saturation->SetComment("trunc/unknown/saturate");
			
			callback = addCallbackHandlerSequence(p_instruction, saturation, detector, p_policy);
			nop->SetFallthrough(callback);
			saturation->SetFallthrough(restore);
		}
		else
		{
			callback = addCallbackHandlerSequence(p_instruction, restore, detector, p_policy);
			nop->SetFallthrough(callback);
		}
	}

	return true;
}

// 8048576 5 INSTR CHECK SIGNEDNESS SIGNED 16 AX ZZ mov     [esp+28h], ax
//             <save flags>
//             TEST ax, ax
//             jns L1
//             invoke callback handler
// (optional)  mov ax, MAX_16     ; saturating arithmetic (Max for bit width/sign/unsigned) 
//  
//       L1:   <restore flags>
//             mov [esp+28h], ax
//
bool IntegerTransform64::addSignednessCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	if (p_annotation.getRegister() == Register::UNKNOWN)
		return false;

	string detector = p_annotation.isSigned() ? SIGNEDNESS64_DETECTOR_SIGNED : SIGNEDNESS64_DETECTOR_UNSIGNED;
	Instruction_t *origFallthrough = p_instruction->GetFallthrough();

	Instruction_t* originalInstrumentInstr = IRDBUtility::insertAssemblyBefore(getFileIR(), p_instruction, std::string("lea rsp, [rsp-128]"), NULL);
	Instruction_t *save = addNewAssembly(p_instruction, "pushf");
	// TEST <reg>, <reg>
	std::ostringstream s;
	s << "test " << Register::toString(p_annotation.getRegister()) << "," << Register::toString(p_annotation.getRegister());
	Instruction_t *test = addNewAssembly(save, s.str());
	originalInstrumentInstr->SetFallthrough(origFallthrough);

	Instruction_t *jns = addNewAssembly(test, "jns 0x22");
	Instruction_t *nop = addNewAssembly(jns, "nop");
	Instruction_t *restore = addNewAssembly("popf");
	Instruction_t *learestore = addNewAssembly(restore, "lea rsp, [rsp+128]");
	learestore->SetFallthrough(originalInstrumentInstr);

	jns->SetTarget(restore);

	Instruction_t *callback = NULL; 

	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
	{
		// what value should we saturate with for 64 and 32 bits?
		Instruction_t *saturation = addNewMaxSaturation(nop, p_annotation.getRegister(), p_annotation);

		if (p_annotation.isSigned())
			saturation->SetComment("signedness/signed/saturate");
		else
			saturation->SetComment("signedness/unsigned/saturate");
			
		callback = addCallbackHandlerSequence(p_instruction, saturation, detector, p_policy);
		nop->SetFallthrough(callback);
		saturation->SetFallthrough(restore);
	}
	else
	{
		callback = addCallbackHandlerSequence(p_instruction, restore, detector, p_policy);
		nop->SetFallthrough(callback);
	}
	
	return true;
}
