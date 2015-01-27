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

//
// Class to handle one MEDS limited function pointer shadow annotation
//
class MEDS_FPTRShadowAnnotation : public MEDS_ShadowAnnotation
{
	public:
		MEDS_FPTRShadowAnnotation();
		MEDS_FPTRShadowAnnotation(const string& p_rawLine);
		virtual ~MEDS_FPTRShadowAnnotation() {}
		virtual const string toString() const { return "fptr shadow: " + m_rawInputLine; }

		bool isRIPRelative() const;
		uintptr_t computeRIPAddress();
		
		const Register::RegisterName getRegister() const;
		const string& getExpression() const { return m_expression; }

	private:
		void parse();
		void setExpression(const string p_expression) { 
			m_expression = p_expression;
			std::transform(m_expression.begin(), m_expression.end(), m_expression.begin(), ::tolower);
		}
		bool verifyCheckShadowExpression(const string& expression);
		int parseRegisterOffset(const char*);
		void parseRegister(const char *p_buf, Register::RegisterName *p_register, int *p_registerOffset);

	private:
		string m_rawInputLine;
		string m_expression;
};

}

#endif
