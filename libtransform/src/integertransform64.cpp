#include <assert.h>
#include <sstream>
#include "leapattern.hpp"
#include "integertransform64.hpp"

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

IntegerTransform64::IntegerTransform64(VariantID_t *p_variantID, FileIR_t *p_fileIR, std::multimap<VirtualOffset, MEDS_InstructionCheckAnnotation> *p_annotations, set<std::string> *p_filteredFunctions, set<VirtualOffset> *p_benignFalsePositives) : IntegerTransform(p_variantID, p_fileIR, p_annotations, p_filteredFunctions, p_benignFalsePositives)
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
				std::pair<std::multimap<VirtualOffset, MEDS_InstructionCheckAnnotation>::iterator,std::multimap<VirtualOffset, MEDS_InstructionCheckAnnotation>::iterator> ret;
				ret = getAnnotations()->equal_range(vo);
				MEDS_InstructionCheckAnnotation annotation; 
				for (std::multimap<VirtualOffset,MEDS_InstructionCheckAnnotation>::iterator it = ret.first; it != ret.second; ++it)
				{
					annotation = it->second;
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
					handleOverflowCheck(insn, annotation, policy);
				}
				else if (annotation.isUnderflow())
				{
					handleUnderflowCheck(insn, annotation, policy);
				}
				else if (annotation.isTruncation())
				{
					handleTruncation(insn, annotation, policy);
				}
			}
		} // end iterate over all instructions in a function
	} // end iterate over all functions

	return 0;
}

void IntegerTransform64::handleOverflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	if (isMultiplyInstruction(p_instruction) || (p_annotation.isOverflow() && !p_annotation.isUnknownSign()) && !p_annotation.isNoFlag())
	{
		// handle signed/unsigned add/sub overflows (non lea)
		addOverflowUnderflowCheck(p_instruction, p_annotation, p_policy);
	}
	else if (p_annotation.isNoFlag())
	{
		// handle lea
		addOverflowCheckNoFlag(p_instruction, p_annotation, p_policy);
	}
	else
	{
		m_numOverflowsSkipped++;
		logMessage(__func__, "OVERFLOW type not yet handled");
	}
}

void IntegerTransform64::handleUnderflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
        if (p_annotation.isUnderflow() && !p_annotation.isNoFlag())
        {
                addOverflowUnderflowCheck(p_instruction, p_annotation, p_policy);
        }
        else
        {
                m_numUnderflowsSkipped++;
                logMessage(__func__, "UNDERFLOW type not yet handled");
        }
}

void IntegerTransform64::handleTruncation(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	if (!isMovInstruction(p_instruction))
	{
		logMessage(__func__, "We only instrument MOV instructions for TRUNCATION annotations");
		m_numTruncationsSkipped++;
		return;
	}

	if (p_annotation.getTruncationFromWidth() == 32)
	{
		if (p_annotation.getTruncationToWidth() == 16 || p_annotation.getTruncationToWidth() == 8)
		{
			addTruncationCheck32(p_instruction, p_annotation, p_policy);
		}
		else
		{
			m_numTruncationsSkipped++;
			logMessage(__func__, "Truncation type not yet handled (1)");
		}
	}
	else
	{
		m_numTruncationsSkipped++;
		logMessage(__func__, "Truncation type not yet handled (2)");
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
void IntegerTransform64::addOverflowUnderflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	char tmpbuf[1024];

	assert(getFileIR() && p_instruction && p_instruction->GetFallthrough());
	Register::RegisterName targetReg = getTargetRegister(p_instruction);
	if (targetReg == Register::UNKNOWN)
	{
		logMessage(__func__, p_annotation, "unknown target register");
		return;
	}
	else if (targetReg == Register::RSP || targetReg == Register::RBP)
	{
		logMessage(__func__, p_annotation, "target register is RSP or RBP: skip");
		return;
	}

	logMessage(__func__, p_annotation, "debug");

	Instruction_t* jncond_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());
	Instruction_t* lea_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());
	Instruction_t* policy_i;

	Instruction_t* next_i = p_instruction->GetFallthrough();
	p_instruction->SetFallthrough(jncond_i); 

	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
		policy_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());
	else
		policy_i = lea_i;

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
		addJnc(jnc_i, policy_i, jncond_i); 
		addJno(jncond_i, policy_i, next_i); 
	}

	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
	{
/*
		if (isMultiplyInstruction(p_instruction) && 
			p_annotation.getBitWidth() == 64 &&
			p_annotation.isSigned())
		{
			saturateSignedMultiplyOverflow(p_instruction, policy_i, lea_i);
		}
		else
*/
		// @todo:
		//    saturating signed multiply overflow should take into account
		//    the signs of the operand. If multiplying negative by positive #,
		//    then we want to sature to MIN_SIGNED_INT
		
		if (p_annotation.isUnderflow())
               		addMinSaturation(policy_i, targetReg, p_annotation, lea_i); 
		else
       	         	addMaxSaturation(policy_i, targetReg, p_annotation, lea_i);
	}

	setAssembly(lea_i, "lea rsp, [rsp-128]");  // red zone 

	// add callback handler here for diagnostics
	//    pass in PC of instrumented instruction
	//    pass in p_policy 
	sprintf(tmpbuf,"push 0x%08x", p_policy);  
	Instruction_t* instr = addNewAssembly(lea_i, tmpbuf);
	sprintf(tmpbuf,"push 0x%08x", p_instruction->GetAddress()->GetVirtualOffset()); 
	instr = addNewAssembly(instr, tmpbuf);

	Instruction_t* call = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());
	instr->SetFallthrough(call);
//	setAssembly(call, "db 0xe8, 00, 00, 00, 00"); 
	setAssembly(call, "call 0"); 
		
	if (p_annotation.isOverflow())
		addCallbackHandler64(call, OVERFLOW64_DETECTOR, 2); // 2 args for now
	else
		addCallbackHandler64(call, UNDERFLOW64_DETECTOR, 2); // 2 args for now

	assert(call->GetTarget());

	instr = addNewAssembly(call, "lea rsp, [rsp+128+16]");  
	call->SetFallthrough(instr);
	instr->SetFallthrough(next_i);

	if (p_annotation.isUnderflow())
		m_numUnderflows++;
	else
		m_numOverflows++;
}

#ifdef XXX
// pre: p_instruction is already allocated
// need to get all registers for the original instruction
void IntegerTransform64::saturateSignedMultiplyOverflow(Instruction_t *p_orig, Instruction_t *p_instruction, Instruction_t* p_fallthrough)
{
	std::set<Register::RegisterName> used;
	Register::RegisterName targetReg = getTargetRegister(p_orig);
	used.insert(targetReg);
	used.insert(getTargetRegister(p_orig,2));
	used.insert(getTargetRegister(p_orig,3));
	used.insert(Register::RDX);

	Register::RegisterName freeReg = Register::getFreeRegister64(used);

	string targetR = Register::toString(targetReg);
	string freeR = Register::toString(freeReg);

        logMessage(__func__, "targetReg: " + targetR + " freeReg: " + freeR);

	Instruction_t *instr;

// wrong instruction code sequence -- ignore

	setAssembly(p_instruction, "push rdx");
	instr = addNewAssembly(p_instruction, "push " + freeR);
/*
	instr = addNewAssembly(p_instruction, "push " + freeR);
	p_instruction->SetFallthrough(instr);
*/
	instr = addNewAssembly(instr, "pushf");
//	instr = addNewAssembly(instr, string("mov rdx, 0x7FFFFFFFFFFFFFFF");
//	instr = addNewAssembly(instr, "shr " + targetR + ", 63");
	instr = addNewAssembly(instr, "shr rdx, 63");
//	instr = addNewAssembly(instr, "add " + targetR + "," + freeR);
	instr = addNewAssembly(instr, "mov " + freeR + ", 0x7FFFFFFFFFFFFFFF");
	instr = addNewAssembly(instr, "add " + freeR + ", rdx");
	instr = addNewAssembly(instr, "mov " + targetR + ", " + freeR);
	instr = addNewAssembly(instr, "popf");
	instr = addNewAssembly(instr, "pop " + freeR);
	instr = addNewAssembly(instr, "pop rdx");
	instr->SetFallthrough(p_fallthrough);
}
#endif

void IntegerTransform64::addOverflowCheckNoFlag(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	LEAPattern leaPattern(p_annotation);

	if (!leaPattern.isValid())
	{
		logMessage(__func__, "invalid or unhandled lea pattern - skipping: ");
		m_numOverflowsSkipped++;
		return;
	}

	if (leaPattern.getRegister1() == Register::UNKNOWN ||
		leaPattern.getRegister1() == Register::RSP || 
		leaPattern.getRegister1() == Register::RBP)
	{
		logMessage(__func__, "destination register is unknown, esp or ebp -- skipping: ");
		m_numOverflowsSkipped++;
		return;
	}
	
	if (leaPattern.isRegisterPlusRegister() || leaPattern.isRegisterTimesRegister())
	{
		Register::RegisterName reg1 = leaPattern.getRegister1();
		Register::RegisterName reg2 = leaPattern.getRegister2();
		Register::RegisterName target = getTargetRegister(p_instruction);

		if (reg1 == Register::UNKNOWN || reg2 == Register::UNKNOWN || target == Register::UNKNOWN)
		{
			logMessage(__func__, "lea reg reg pattern: error retrieving register: reg1: " + Register::toString(reg1) + " reg2: " + Register::toString(reg2) + " target: " + Register::toString(target));
			m_numOverflowsSkipped++;
			return;
		}
		else if (reg2 == Register::RSP || target == Register::RSP) 
		{
			logMessage(__func__, "source or target register is rsp -- skipping: ");
			m_numOverflowsSkipped++;
			return;
		}
		else if (p_annotation.getBitWidth() != 64)
		{
			logMessage(__func__, "we only handle 64 bit LEAs for now:" + p_annotation.toString());
			m_numOverflowsSkipped++;
			return;
		}
		else if (Register::getBitWidth(target) != 64)
		{
			logMessage(__func__, "we only handle 64 bit registers in LEAs for now: " + p_annotation.toString());
			m_numOverflowsSkipped++;
			return;
		}
		else
		{
			if (leaPattern.isRegisterPlusRegister())
				addOverflowCheckNoFlag_RegPlusReg(p_instruction, p_annotation, reg1, reg2, target, p_policy);
			else if (leaPattern.isRegisterTimesRegister())
				addOverflowCheckNoFlag_RegTimesReg(p_instruction, p_annotation, reg1, reg2, target, p_policy);
		}
		return;
	}
	else if (leaPattern.isRegisterPlusConstant() || leaPattern.isRegisterTimesConstant())
	{
		Register::RegisterName reg1 = leaPattern.getRegister1();
		int k = leaPattern.getConstant();
		Register::RegisterName target = getTargetRegister(p_instruction);

		if (reg1 == Register::UNKNOWN || target == Register::UNKNOWN)
		{
			logMessage(__func__, "lea reg reg pattern: error retrieving register: reg1: " + Register::toString(reg1) + " target: " + Register::toString(target));
			m_numOverflowsSkipped++;
			return;
		}
		else if (target == Register::RSP) 
		{
			logMessage(__func__, "target register is rsp -- skipping: ");
			m_numOverflowsSkipped++;
			return;
		}
		else if (p_annotation.getBitWidth() != 64)
		{
			logMessage(__func__, "we only handle 64 bit LEAs for now:" + p_annotation.toString());
			m_numOverflowsSkipped++;
			return;
		}
		else if (Register::getBitWidth(target) != 64)
		{
			logMessage(__func__, "we only handle 64 bit registers in LEAs for now: " + p_annotation.toString());
			m_numOverflowsSkipped++;
			return;
		}
		else
		{
			if (leaPattern.isRegisterPlusConstant())
				addOverflowCheckNoFlag_RegPlusConstant(p_instruction, p_annotation, reg1, k, target, p_policy);
			else if (leaPattern.isRegisterTimesConstant())
				addOverflowCheckNoFlag_RegTimesConstant(p_instruction, p_annotation, reg1, k, target, p_policy);
		}
		return;
	}

	m_numOverflowsSkipped++;
	logMessage(__func__, "not yet handling lea -- placeholder");
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

	// pass in PC of instrumented instruction
	// pass in p_policy 
	sprintf(tmpbuf,"push 0x%08x", p_policy);  
	Instruction_t* instr = addNewAssembly(lea, tmpbuf);
	sprintf(tmpbuf,"push 0x%08x", p_orig->GetAddress()->GetVirtualOffset()); 
	instr = addNewAssembly(instr, tmpbuf);

	Instruction_t* call = allocateNewInstruction(p_orig->GetAddress()->GetFileID(), p_orig->GetFunction());
	instr->SetFallthrough(call);
	setAssembly(call, "call 0"); 
		
	addCallbackHandler64(call, p_detector, 2); // 2 args for now

	assert(call->GetTarget());

	instr = addNewAssembly(call, "lea rsp, [rsp+128+16]");  
	call->SetFallthrough(instr);
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
	saturation_policy = addNewAssembly("nop");
	saturation_policy->SetComment("lea overflow instrumentation(reg+reg): policy code sequence");
	Instruction_t *popf = addNewAssembly("popf");
	Instruction_t *callback = addCallbackHandlerSequence(p_instruction, popf, OVERFLOW64_DETECTOR, p_policy);
	saturation_policy->SetFallthrough(callback);
	instr = addNewAssembly(popf, "pop " + Register::toString(p_reg1));
	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
	{
		instr = addNewMaxSaturation(instr, p_reg3, p_annotation);
	}

	instr->SetFallthrough(origFallthrough);

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
	first = addNewAssembly("push " + Register::toString(p_reg1)); 
	first->SetComment("lea overflow instrumentation(reg+reg): start");
	instr = addNewAssembly(first, "pushf");
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, first);

	// make sure we set the fallthrough post careful insertion
	first->SetFallthrough(instr);

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

	// <restore>
	//         popf                   ; restore flags
	//         pop r1                 ; restore register
	//
	// <orig>: lea r3, [r1+r2]        ; original instruction        
	//         <originalNext>         ; original next instruction
	//
	restore->SetComment("lea overflow instrumentation(reg+reg): restore");
	instr = addNewAssembly(restore, "pop " + Register::toString(p_reg1));

	instr->SetFallthrough(originalInstrumentInstr);
	m_numOverflows++;
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
	saturation_policy = addNewAssembly("nop");
	saturation_policy->SetComment("lea overflow instrumentation(reg*reg): policy code sequence");
	Instruction_t *popf = addNewAssembly("popf");
	Instruction_t *callback = addCallbackHandlerSequence(p_instruction, popf, OVERFLOW64_DETECTOR, p_policy);
	saturation_policy->SetFallthrough(callback);
	instr = addNewAssembly(popf, "pop " + Register::toString(p_reg1));
	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC) 
	{
		instr = addNewMaxSaturation(instr, p_reg3, p_annotation);
	}

	instr->SetFallthrough(origFallthrough);

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
	first = addNewAssembly("push " + Register::toString(p_reg1)); 
	first->SetComment("lea overflow instrumentation(reg*reg): start");
	instr = addNewAssembly(first, "pushf");
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, first);

	// make sure we set the fallthrough post careful insertion
	first->SetFallthrough(instr);

	instr = addNewAssembly(instr, "imul " + Register::toString(p_reg1) + "," + Register::toString(p_reg2));
	instr = addNewAssembly(instr, "jno 0x22");
	instr->SetFallthrough(saturation_policy);
	instr->SetTarget(restore);

	// <restore>
	//         popf                   ; restore flags
	//         pop r1                 ; restore register
	//
	// <orig>: lea r3, [r1*r2]        ; original instruction        
	//         <originalNext>         ; original next instruction
	//
	restore->SetComment("lea overflow instrumentation(reg*reg): restore");
	instr = addNewAssembly(restore, "pop " + Register::toString(p_reg1));

	instr->SetFallthrough(originalInstrumentInstr);
	m_numOverflows++;
}

// Example annotation to handle
// 804852e      3 INSTR CHECK OVERFLOW NOFLAGSIGNED 32 EDX+EAX ZZ lea     eax, [edx+eax] Reg1: EDX Reg2: EAX
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

	cerr << __func__ << ": r3 <-- r1+k: r1: " << Register::toString(p_reg1) << " k: " << p_constant << " target register: " << Register::toString(p_reg3) << "  annotation: " << p_annotation.toString() << endl;

	Instruction_t *origFallthrough = p_instruction->GetFallthrough();
	Instruction_t *instr, *first, *saturation_policy;
	Instruction_t *restore = addNewAssembly("popf");

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
	saturation_policy = addNewAssembly("nop");
	saturation_policy->SetComment("lea overflow instrumentation(reg+k): policy code sequence");
	Instruction_t *popf = addNewAssembly("popf");
	Instruction_t *callback = addCallbackHandlerSequence(p_instruction, popf, OVERFLOW64_DETECTOR, p_policy);
	saturation_policy->SetFallthrough(callback);
	instr = addNewAssembly(popf, "pop " + Register::toString(p_reg1));
	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC) 
	{
		instr = addNewMaxSaturation(instr, p_reg3, p_annotation);
	}

	instr->SetFallthrough(origFallthrough);

	// Original code sequence:
	//   lea r3, [r1+r2]
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
	first = addNewAssembly("push " + Register::toString(p_reg1)); 
	first->SetComment("lea overflow instrumentation(reg+k): start");
	instr = addNewAssembly(first, "pushf");
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, first);

	// make sure we set the fallthrough post careful insertion
	first->SetFallthrough(instr);

	std::ostringstream s;
	s << "add " << Register::toString(p_reg1) << "," << p_constant;
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

	// <restore>
	//         popf                   ; restore flags
	//         pop r1                 ; restore register
	//
	// <orig>: lea r3, [r1+k]         ; original instruction        
	//         <originalNext>         ; original next instruction
	//
	restore->SetComment("lea overflow instrumentation(reg+k): restore");
	instr = addNewAssembly(restore, "pop " + Register::toString(p_reg1));

	instr->SetFallthrough(originalInstrumentInstr);
	m_numOverflows++;
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
	saturation_policy = addNewAssembly("nop");
	saturation_policy->SetComment("lea overflow instrumentation(reg*k): policy code sequence");
	Instruction_t *popf = addNewAssembly("popf");
	Instruction_t *callback = addCallbackHandlerSequence(p_instruction, popf, OVERFLOW64_DETECTOR, p_policy);
	saturation_policy->SetFallthrough(callback);
	instr = addNewAssembly(popf, "pop " + Register::toString(p_reg1));
	if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC) 
	{
		instr = addNewMaxSaturation(instr, p_reg3, p_annotation);
	}

	instr->SetFallthrough(origFallthrough);

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
	first = addNewAssembly("push " + Register::toString(p_reg1)); 
	first->SetComment("lea overflow instrumentation(reg*k): start");
	instr = addNewAssembly(first, "pushf");
	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, first);

	// make sure we set the fallthrough post careful insertion
	first->SetFallthrough(instr);

	std::ostringstream s;
	s << "imul " << Register::toString(p_reg1) << "," << p_constant;
	instr = addNewAssembly(instr, s.str());
	instr = addNewAssembly(instr, "jno 0x22");
	instr->SetFallthrough(saturation_policy);
	instr->SetTarget(restore);

	// <restore>
	//         popf                   ; restore flags
	//         pop r1                 ; restore register
	//
	// <orig>: lea r3, [r1*k]        ; original instruction        
	//         <originalNext>         ; original next instruction
	//
	restore->SetComment("lea overflow instrumentation(reg*reg): restore");
	instr = addNewAssembly(restore, "pop " + Register::toString(p_reg1));

	instr->SetFallthrough(originalInstrumentInstr);
	m_numOverflows++;
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

void IntegerTransform64::addTruncationCheck32(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
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

	p_instruction->SetComment("monitor for truncation");

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

	save = addNewAssembly("pushf");
	save->SetComment("start truncation sequence");

	if (p_annotation.isSigned())
	{
		sprintf(buf, "test %s, 0x%8x", Register::toString(p_annotation.getRegister()).c_str(), mask2);
	}
	else
	{
		sprintf(buf, "test %s, 0x%8x", Register::toString(p_annotation.getRegister()).c_str(), mask);
	}
	logMessage(__func__, buf);
	test = addNewAssembly(save, buf);

	Instruction_t* originalInstrumentInstr = carefullyInsertBefore(p_instruction, save);

	save->SetFallthrough(test);

	restore->SetFallthrough(originalInstrumentInstr);

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
		s << "cmp " << Register::toString(p_annotation.getRegister()) << "," << mask2;
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

	m_numTruncations++;
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
