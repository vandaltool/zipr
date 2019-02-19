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

#ifndef _MEDS_FUNCANNOTATION_H_
#define _MEDS_FUNCANNOTATION_H_

#include <string>
#include "VirtualOffset.hpp"
#include "MEDS_Register.hpp"
#include "MEDS_AnnotationBase.hpp"


namespace MEDS_Annotation 
{

// These strings must match those emitted by MEDS in the information annotation file exactly
#define MEDS_ANNOT_FUNC        "FUNC"

using namespace std;
using namespace MEDS_Annotation;

//
// Class to handle one MEDS annotation
//
class MEDS_FuncAnnotation : public MEDS_AnnotationBase
{
	public:
		MEDS_FuncAnnotation() {}
		virtual ~MEDS_FuncAnnotation(){}

		virtual bool isFuncAnnotation() const { return true; } 

		virtual string getFuncName() const { return m_func_name; }
		virtual void setFuncName(const string &p_func_name) { m_func_name=p_func_name; }

		virtual bool isLeaf() const { return m_isleaf; }
		virtual void setLeaf(const bool p_leaf) { m_isleaf = p_leaf; }

		virtual bool hasFramePointer() const { return m_hasfp; }
		virtual void setHasFramePointer(const bool p_hasfp) { m_hasfp = p_hasfp; }

	private:
		string m_func_name;
		bool m_isleaf;
		bool m_hasfp;
};

}
#endif
