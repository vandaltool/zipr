/*
 * Copyright (c) 2015 - Zephyr Software LLC
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software LLC
 * URL   : http://www.zephyr-software.com/
 *
 */

#include <assert.h>
#include <sstream>
#include "pointercheck64.hpp"

using namespace libTransform;


PointerCheck64::PointerCheck64(VariantID_t *p_variantID, FileIR_t *p_fileIR, MEDS_Annotations_t *p_annotations, set<std::string> *p_filteredFunctions, set<VirtualOffset> *p_benignFalsePositives) : IntegerTransform64(p_variantID, p_fileIR, p_annotations, p_filteredFunctions, p_benignFalsePositives)
{
}

//
// p_orig          original instruction being instrumented
// p_fallthrough   fallthrough once we're done with the callback handlers/detectors
// p_detector      callback handler function to call
// p_policy        instrumentation policy (e.g.: terminate, saturate, ...)
// 
// returns first instruction of callback handler sequence
//
Instruction_t* PointerCheck64::addCallbackHandlerSequence(Instruction_t *p_orig, Instruction_t *p_fallthrough, std::string p_detector, int p_policy)
{
	if (p_policy == POLICY_EXIT)
	{				
		Instruction_t* exit_sequence = addNewAssembly("mov rax, 60");
		Instruction_t* i = addNewAssembly(exit_sequence, "mov rdi, 25");
		i = addNewAssembly(i, "sysenter");
		i->SetFallthrough(p_fallthrough); 
		return exit_sequence;
	}
	else
		return IntegerTransform64::addCallbackHandlerSequence(p_orig, p_fallthrough, p_detector, p_policy);
}

int PointerCheck64::execute()
{
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
				int policy = POLICY_EXIT; 

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

				m_numAnnotations++;

				if (!annotation.isIdiom() || annotation.getIdiomNumber() != IDIOM_POINTER_NUMERIC_WEAKNESS)
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

				// this idiom deals with pointers ==> unsigned
				if (!annotation.isUnsigned())
				{							
					logMessage(__func__, "Warning: this idiom should only see unsigned annotations -- force to UNSIGNED");
					annotation.setUnsigned();
				}

				logMessage(__func__, annotation, "-- instruction: " + insn->getDisassembly());

				if (annotation.isOverflow())
				{
					m_numTotalOverflows++;
					handleOverflowCheck(insn, annotation, policy);
				}

				if (annotation.isUnderflow())
				{
					m_numTotalUnderflows++;
					handleUnderflowCheck(insn, annotation, policy);
				}
			}
		} // end iterate over all instructions in a function
	} // end iterate over all functions

	return 0;
}

