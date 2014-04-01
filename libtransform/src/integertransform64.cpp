#include <assert.h>
#include "integertransform64.hpp"

using namespace libTransform;

/**
*     64 bit implementation status of the integer transform
*
*     20140228 64-bit overflows on multiply, signed/unsigned add/sub, halt policy
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
	if (isMultiplyInstruction(p_instruction) || (p_annotation.isOverflow() && !p_annotation.isUnknownSign()))
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
        if (p_annotation.isUnderflow())
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
// Halting Policy
//	        mul a, b                 ; <instruction to instrument>
//              jno <OrigNext>           ; if no overflows, jump to original fallthrough instruction
//              hlt                      ; policy = halt 
// OrigNext:    <nextInstruction>        ; original fallthrugh
//
// Saturation Policy
//	        mul a, b                 ; <instruction to instrument>
//              jno <OrigNext>           ; if no overflows, jump to original fallthrough instruction
//              movq a, MIN/MAX          ; policy = min-saturate (underflow) / max-saturate (overflow)
// OrigNext:    <nextInstruction>        ; original fallthrugh
//
void IntegerTransform64::addOverflowUnderflowCheck(Instruction_t *p_instruction, const MEDS_InstructionCheckAnnotation& p_annotation, int p_policy)
{
	Instruction_t* jncond_i = NULL;
	Instruction_t* policy_i = NULL;

	assert(getFileIR() && p_instruction && p_instruction->GetFallthrough());

	Register::RegisterName targetReg = getTargetRegister(p_instruction);
        if (targetReg == Register::UNKNOWN)
        {
                logMessage(__func__, p_annotation, "unknown target register");
		return;
	}

//	cerr << __func__ <<  ": instr: " << p_instruction->getDisassembly() << " address: " << std::hex << p_instruction->GetAddress() << " annotation: " << p_annotation.toString() << " policy: " << p_policy << endl;

        logMessage(__func__, p_annotation, "debug");

	jncond_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());
	policy_i = allocateNewInstruction(p_instruction->GetAddress()->GetFileID(), p_instruction->GetFunction());

	Instruction_t* next_i = p_instruction->GetFallthrough();
	p_instruction->SetFallthrough(jncond_i); 

	if (p_annotation.isUnsigned())
	{
		addJnc(jncond_i, policy_i, next_i);
	}
	else
	{
		addJno(jncond_i, policy_i, next_i);
	}

        if (p_policy == POLICY_CONTINUE_SATURATING_ARITHMETIC)
	{
		if (p_annotation.isUnderflow())
		{
                	addMinSaturation(policy_i, targetReg, p_annotation, next_i);
		}
		else
		{
                	addMaxSaturation(policy_i, targetReg, p_annotation, next_i);
		}
	}
	else
	{
		// would need to min-saturate for underflow
		//               max-saturate for overflow
		addHlt(policy_i, next_i);
	}

	if (p_annotation.isUnderflow())
		m_numUnderflows++;
	else
		m_numOverflows++;
}

