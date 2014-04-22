#include <assert.h>
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
*        mul          of          of        of
*        lea         @todo      @todo       @todo
*             
*      32-bit        signed     unsigned   unknown
*      --------      ------     --------   -------
*      add, sub      @todo      @todo       @todo
*        mul         @todo      @todo       @todo
*        lea         @todo      @todo       @todo

*     @todo:
*           - test unknown
*           - test warnings only mode
*           - handle LEA instructions
*
**/

IntegerTransform64::IntegerTransform64(VariantID_t *p_variantID, FileIR_t *p_fileIR, std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> *p_annotations, set<std::string> *p_filteredFunctions, set<VirtualOffset> *p_benignFalsePositives) : IntegerTransform(p_variantID, p_fileIR, p_annotations, p_filteredFunctions, p_benignFalsePositives)
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

				MEDS_InstructionCheckAnnotation annotation = (*getAnnotations())[vo];
				if (!annotation.isValid()) 
					continue;

				logMessage(__func__, annotation, "-- instruction: " + insn->getDisassembly());
				m_numAnnotations++;

				if (annotation.isIdiom())
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
	else
	{
		m_numOverflowsSkipped++;
		logMessage(__func__, "OVERFLOW type not yet handled");
	}
}

void IntegerTransform64::handleUnderflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
        if (p_annotation.isUnderflow() && p_annotation.isNoFlag())
        {
                addOverflowUnderflowCheck(p_instruction, p_annotation, p_policy);
        }
        else
        {
                m_numUnderflowsSkipped++;
                logMessage(__func__, "UNDERFLOW type not yet handled");
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
	// pass in PC of instrumented instruction
	// pass in p_policy 
	sprintf(tmpbuf,"push 0x%08x", p_policy);  
	Instruction_t* instr = addNewAssembly(lea_i, tmpbuf);
	sprintf(tmpbuf,"push 0x%08x", p_instruction->GetAddress()->GetVirtualOffset()); 
	instr = addNewAssembly(instr, tmpbuf);

	Instruction_t* call = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());
	instr->SetFallthrough(call);
//	setAssembly(call, "db 0xe8, 00, 00, 00, 00"); 
	setAssembly(call, "call 0"); 
		
	if (p_annotation.isOverflow())
		addCallbackHandler64(call, OVERFLOW_DETECTOR_64, 2); // 2 args for now
	else
		addCallbackHandler64(call, UNDERFLOW_DETECTOR_64, 2); // 2 args for now

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
