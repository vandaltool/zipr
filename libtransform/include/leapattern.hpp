/*
 * Copyright (c) 2013, 2014 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

#ifndef _LEA_PATTERN_H_
#define _LEA_PATTERN_H_

#include <regex.h>

#include "MEDS_InstructionCheckAnnotation.hpp"
#include "MEDS_Register.hpp"

using namespace MEDS_Annotation;

namespace libTransform {

class LEAPattern {
	public:
		LEAPattern(const MEDS_InstructionCheckAnnotation& p_annotation);
		bool isValid() const;

		bool isRegisterPlusRegister() const;
		bool isRegisterPlusConstant() const;
		bool isRegisterTimesConstant() const; 
		bool isRegisterTimesRegister() const; // not used

		Register::RegisterName getRegister1() const;
		Register::RegisterName getRegister2() const;
		int getConstant() const;

	private:
		void extractOperands(const string target, string& op1, string& op2);

	private:
		bool                     m_isValid;
		bool                     m_isRegPlusReg;
		bool                     m_isRegPlusConstant;
		bool                     m_isRegTimesReg;
		bool                     m_isRegTimesConstant;
		Register::RegisterName   m_reg1;
		Register::RegisterName   m_reg2;
		int                      m_constant;

		regex_t                  m_regex_reg_plus_reg;
		regex_t                  m_regex_reg_times_reg;
		regex_t                  m_regex_reg_plus_constant;
		regex_t                  m_regex_reg_plus_negconstant;
		regex_t                  m_regex_reg_times_constant;
};

}

#endif
