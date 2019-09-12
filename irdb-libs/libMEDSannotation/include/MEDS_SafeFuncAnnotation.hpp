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

#ifndef _MEDS_SAFEFUNCANNOTATION_H_
#define _MEDS_SAFEFUNCANNOTATION_H_

#include <string>
#include "VirtualOffset.hpp"
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
class MEDS_SafeFuncAnnotation : public MEDS_FuncAnnotation
{
	public:
		MEDS_SafeFuncAnnotation() {m_safe_func=false;}
		MEDS_SafeFuncAnnotation(const string &p_rawLine);
		virtual ~MEDS_SafeFuncAnnotation(){}

		virtual const string toString() const { return "safe func: "+m_rawInputLine; }

		virtual void markSafe() { m_safe_func = true; setValid(); }
		virtual void markUnsafe() { m_safe_func = false; setValid(); }
		virtual bool isSafe() { return m_safe_func; }


	private:
		void init();
		void parse();

	private:
		std::string m_rawInputLine;

		bool m_safe_func;
};

}
#endif
