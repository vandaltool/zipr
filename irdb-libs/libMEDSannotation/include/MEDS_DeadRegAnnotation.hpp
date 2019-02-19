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

#ifndef _MEDS_DEADREGSANNOTATION_H_
#define _MEDS_DEADREGSANNOTATION_H_

#include <algorithm>
#include <string>
#include <stdint.h>
#include "VirtualOffset.hpp"
#include "MEDS_Register.hpp"
#include "MEDS_AnnotationBase.hpp"

namespace MEDS_Annotation 
{

using namespace std;
using namespace MEDS_Annotation;


//
// Class to handle one MEDS limited function pointer shadow annotation
//
class MEDS_DeadRegAnnotation : public MEDS_AnnotationBase
{
	public:
		MEDS_DeadRegAnnotation();
		MEDS_DeadRegAnnotation(const string& p_rawLine);
		virtual ~MEDS_DeadRegAnnotation() {}
		virtual const string toString() const { return "deadregs: " + m_rawInputLine; }

		RegisterSet_t& getRegisterSet() { return regset; }

	private:
		// base class requirements.
		void parse();

	private:
		RegisterSet_t regset;
		string m_rawInputLine;
};

}

#endif
