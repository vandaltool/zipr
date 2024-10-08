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

#ifndef _MEDS_INSTRUCTIONCHECKANNOTATION_H_
#define _MEDS_INSTRUCTIONCHECKANNOTATION_H_

#include <string>
#include "VirtualOffset.hpp"
#include "MEDS_Register.hpp"
#include "MEDS_AnnotationBase.hpp"


namespace MEDS_Annotation 
{

// These strings must match those emitted by MEDS in the information annotation file exactly
#define MEDS_ANNOT_INSTR        "INSTR"
#define MEDS_ANNOT_CHECK        "CHECK"
#define MEDS_ANNOT_OVERFLOW     "OVERFLOW"
#define MEDS_ANNOT_UNDERFLOW    "UNDERFLOW"
#define MEDS_ANNOT_SIGNEDNESS   "SIGNEDNESS"
#define MEDS_ANNOT_TRUNCATION   "TRUNCATION"
#define MEDS_ANNOT_SIGNED       "SIGNED"
#define MEDS_ANNOT_UNSIGNED     "UNSIGNED"
#define MEDS_ANNOT_UNKNOWNSIGN  "UNKNOWNSIGN"
#define MEDS_ANNOT_NOFLAG       "NOFLAG"
#define MEDS_ANNOT_INFINITELOOP "INFINITELOOP"
#define MEDS_ANNOT_SEVERE       "SEVERE"
#define MEDS_ANNOT_FLOWS_INTO_CRITICAL_SINK  "SINKMALLOC"
#define MEDS_ANNOT_MEMSET       "MEMSET"
#define MEDS_ANNOT_IDIOM        "IDIOM"

using namespace std;
using namespace MEDS_Annotation;

//
// Class to handle one MEDS (integer vulnerability) annotation
//
class MEDS_InstructionCheckAnnotation : public MEDS_AnnotationBase
{
	public:
		MEDS_InstructionCheckAnnotation();
		MEDS_InstructionCheckAnnotation(const string &p_rawLine);
		virtual ~MEDS_InstructionCheckAnnotation(){}

		// integer vulnerability types
		bool isOverflow() const { return m_isOverflow; }
		bool isUnderflow() const { return m_isUnderflow; }
		bool isTruncation() const { return m_isTruncation; }
		bool isSignedness() const { return m_isSignedness; }
		bool isSevere() const { return m_isSevere; }
		void setOverflow() { m_isOverflow = true; }

		// signed vs. unsigned
		bool isUnsigned() const { return m_isUnsigned; }
		bool isSigned() const { return m_isSigned; }
		bool isUnknownSign() const { return m_isUnknownSign; }

		void setSigned() { m_isSigned = true; }
		void setUnsigned() { m_isUnsigned = true; }
		void setUnknownSign() { m_isUnknownSign = true; }

		// overflow with no flags, e.g. lea
		bool isNoFlag() const { return m_isNoFlag; }

        // infinite loop annotation?
		bool isInfiniteLoop() const { return m_isInfiniteLoop; }

		// memset annotation?
		bool isMemset() const { return m_isMemset; }
		bool isEspOffset() const { return m_isEspOffset; }
		bool isEbpOffset() const { return m_isEbpOffset; }
		int getStackOffset() const { return m_stackOffset; }
		int getObjectSize() const { return m_objectSize; }

		// is idiom?
		bool isIdiom() const { return m_isIdiom; }
		int getIdiomNumber() const { return m_idiomNumber; }
		void setIdiomNumber(int number) { m_idiomNumber = number; }

		// get bitwidth
		int getBitWidth() const { return m_bitWidth; }
		void setBitWidth(int p_bitWidth) { m_bitWidth = p_bitWidth; }
		int getTruncationFromWidth() const { return m_truncationFromWidth; }
		int getTruncationToWidth() const { return m_truncationToWidth; }

		// get register
		MEDS_Annotation::RegisterName getRegister() const { return m_register; }
		MEDS_Annotation::RegisterName getRegister2() const { return m_register2; }

		const string getTarget() const { return m_target; }
		const string getTarget2() const { return m_target2; }

		const string toString() const { return m_rawInputLine; }

		// data flow
		// @todo: expand the set, allow getter functions to retrieve name of sink
		bool flowsIntoCriticalSink() const { return m_flowsIntoCriticalSink; }

	private:
		void init();
		void parse();

	private:
		string         m_rawInputLine;
		bool           m_isOverflow;
		bool           m_isUnderflow;
		bool           m_isSignedness;
		bool           m_isTruncation;
		bool           m_isSigned;
		bool           m_isUnsigned;
		bool           m_isUnknownSign;
		bool           m_isNoFlag;
		bool           m_isInfiniteLoop;
		bool           m_isMemset;
		bool           m_isSevere;
		int            m_bitWidth;
		int            m_truncationFromWidth;
		int            m_truncationToWidth;
		bool           m_isEspOffset;
		bool           m_isEbpOffset;
		int            m_stackOffset;
		int            m_objectSize;
		bool           m_isValid;
		bool           m_flowsIntoCriticalSink;
		bool           m_isIdiom;
		int            m_idiomNumber;
		MEDS_Annotation::RegisterName       m_register;
		MEDS_Annotation::RegisterName       m_register2;
		string         m_target;
		string         m_target2;
};

}
#endif
