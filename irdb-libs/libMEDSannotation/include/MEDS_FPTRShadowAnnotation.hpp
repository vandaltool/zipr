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

#ifndef _MEDS_FPTRSHADOWANNOTATION_H_
#define _MEDS_FPTRSHADOWANNOTATION_H_

#include <algorithm>
#include <string>
#include <stdint.h>
#include "VirtualOffset.hpp"
#include "MEDS_ShadowAnnotation.hpp"
#include "MEDS_Register.hpp"

namespace MEDS_Annotation 
{

using namespace std;
using namespace MEDS_Annotation;


#define MEDS_ANNOT_FPTRSHADOW "FPTRSHADOW"
#define MEDS_ANNOT_FPTRCHECK "FPTRCHECK"
#define MEDS_ANNOT_ARGSHADOW "ARGSHADOW"
#define MEDS_ANNOT_ARGCHECK "ARGCHECK"

//
// Class to handle one MEDS limited function pointer shadow annotation
//
// Examples:
//    8057fa7      3 INSTR FPTRSHADOW  [EAX+8] SHADOWID 5
//    805829d      3 INSTR FPTRCHECK  [EBP-40] SHADOWID 5
//    80822aa      7 INSTR FPTRSHADOW  0 SHADOWID 6
//    80822cc      3 INSTR FPTRSHADOW  EAX SHADOWID 6
//    8480faa      9 INSTR FPTRSHADOW  4721886 SHADOWID 75
//
class MEDS_FPTRShadowAnnotation : public MEDS_ShadowAnnotation
{
	public:
		MEDS_FPTRShadowAnnotation();
		MEDS_FPTRShadowAnnotation(const string& p_rawLine);
		virtual ~MEDS_FPTRShadowAnnotation() {}
		virtual const string toString() const { return "fptr/arg shadow: " + m_rawInputLine; }

		bool isRIPRelative() const;
		uintptr_t computeRIPAddress();
		
                bool isConstant() const;
                long long getConstantValue(bool&) const;

                bool isRegister() const;
                bool isMemoryExpression() const;

		const RegisterName getRegister() const;
		const string& getExpression() const { return m_expression; }

		const bool isFunctionPointerShadow() const { return m_functionPointerShadow; }
		const bool isCriticalArgumentShadow() const { return m_criticalArgumentShadow; }
	private:
		void parse();
		void setExpression(const string p_expression) { 
			m_expression = p_expression;
			std::transform(m_expression.begin(), m_expression.end(), m_expression.begin(), ::tolower);
		}
		bool verifyCheckShadowExpression(const string& expression);
		int parseRegisterOffset(const char*);
		void parseRegister(const char *p_buf, RegisterName *p_register, int *p_registerOffset);

		void setFunctionPointerShadow(const bool p_val) { m_functionPointerShadow = p_val; }
		void setCriticalArgumentShadow(const bool p_val) { m_criticalArgumentShadow = p_val; }

	private:
		string m_rawInputLine;
		string m_expression;
		bool m_functionPointerShadow;
		bool m_criticalArgumentShadow;
};

}

#endif
