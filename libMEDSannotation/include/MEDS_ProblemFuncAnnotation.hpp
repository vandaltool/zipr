/*
 * Copyright (c) 2014 - Zephyr Software LLC
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

#ifndef _MEDS_PROBLEMFUNCANNOTATION_H_
#define _MEDS_PROBLEMFUNCANNOTATION_H_

#include <string>
#include "VirtualOffset.hpp"
#include "MEDS_Register.hpp"
#include "MEDS_FuncAnnotation.hpp"


namespace MEDS_Annotation 
{

// These strings must match those emitted by MEDS in the information annotation file exactly
#define MEDS_ANNOT_FUNC        "FUNC"

using namespace std;
using namespace MEDS_Annotation;

//
// Class to handle one MEDS (integer vulnerability) annotation
//
class MEDS_ProblemFuncAnnotation : public MEDS_FuncAnnotation
{

	public:
		enum ProblemType { pt_CallUnresolved, pt_JumpUnresolved, pt_BadStackAnalysis, pt_BadRTLs, pt_Unknown };

		MEDS_ProblemFuncAnnotation() {};
		MEDS_ProblemFuncAnnotation(const string &p_rawLine);
		virtual ~MEDS_ProblemFuncAnnotation(){}

		virtual const string toString() const { return "problem func: "+m_rawInputLine; }

		virtual void markCallUnresolved() { pt = pt_CallUnresolved; setValid(); }
		virtual void markJumpUnresolved() { pt = pt_JumpUnresolved; setValid(); }
		virtual bool isCallUnresolved() { return pt==pt_CallUnresolved; }
		virtual bool isJumpUnresolved() { return pt==pt_JumpUnresolved; }

		ProblemType getProblemType() { return pt; }
		void setProblemType( ProblemType _pt) { pt=_pt; }

	private:
		void init();
		void parse();
		bool matches(std::string line, string pattern, ProblemType prob_type);

	private:
		std::string m_rawInputLine;

		ProblemType pt;

};

}
#endif
