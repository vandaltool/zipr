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

#include <string>
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
		MEDS_FPTRShadowAnnotation(const string &p_rawLine);
		virtual ~MEDS_FPTRShadowAnnotation() {}
		virtual const string toString() const { return "fptr shadow: " + m_rawInputLine; }

		void setRegister(Register::RegisterName p_reg) { m_register = p_reg; }
		Register::RegisterName getRegister() const { return m_register; }
		void setRegisterOffset(int p_offset) { m_registerOffset = p_offset; }
		int getRegisterOffset() const { return m_registerOffset; }

	private:
		void parse();
		int parseRegisterOffset(char*);
		void parseRegister(char *p_buf, Register::RegisterName *p_register, int *p_registerOffset);

	private:
		std::string m_rawInputLine;
		Register::RegisterName m_register;
		int m_registerOffset;
};

}

#endif
