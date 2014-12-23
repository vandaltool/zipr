/*
 * Copyright (c) 2014 - Zephyr Software
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */

#ifndef FPTRSHADOW_INSTRUMENT64_HPP
#define FPTRSHADOW_INSTRUMENT64_HPP

#include <libIRDB-core.hpp>
#include "MEDS_AnnotationParser.hpp"
#include "MEDS_FPTRShadowAnnotation.hpp"

using namespace MEDS_Annotation;
using namespace libIRDB;

class FPTRShadow_Instrument64
{
	public:
		FPTRShadow_Instrument64(FileIR_t *p_firp, MEDS_AnnotationParser* p_meds_ap);
		bool execute();

	private:
		MEDS_Annotations_t& getAnnotations() ;
		bool addShadowEntry(Instruction_t *p_insn, const MEDS_FPTRShadowAnnotation *p_annot);
		bool checkShadowEntry(Instruction_t *p_insn, const MEDS_FPTRShadowAnnotation *p_annot);

	private:
		FileIR_t* m_firp;
		MEDS_AnnotationParser* m_annotationParser;
};

#endif

