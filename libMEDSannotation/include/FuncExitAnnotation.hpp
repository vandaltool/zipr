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

#ifndef _FUNCEXITANNOTATION_H_
#define _FUNCEXITANNOTATION_H_

#include <string>
#include "VirtualOffset.hpp"
#include "MEDS_Register.hpp"
#include "MEDS_AnnotationBase.hpp"


namespace MEDS_Annotation 
{

using namespace std;
using namespace MEDS_Annotation;

//
// Class to handle one MEDS  annotation
//
class MEDS_FuncExitAnnotation : public MEDS_AnnotationBase
{
	public:
		MEDS_FuncExitAnnotation() {};
		MEDS_FuncExitAnnotation(const string &p_rawLine);
		virtual ~MEDS_FuncExitAnnotation(){}

		virtual const string toString() const { return "tail call: "+m_rawInputLine; }

	private:
		void init();
		void parse();

	private:
		std::string m_rawInputLine;
};

}
#endif
