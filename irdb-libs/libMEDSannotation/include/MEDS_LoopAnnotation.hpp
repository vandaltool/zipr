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

#ifndef _MEDS_LOOPANNOTATION_H_
#define _MEDS_LOOPANNOTATION_H_

#include <string>
#include "VirtualOffset.hpp"
#include "MEDS_Register.hpp"
#include <set>
#include "MEDS_AnnotationBase.hpp"


namespace MEDS_Annotation 
{

using namespace std;
using namespace MEDS_Annotation;

//
// Class to handle one MEDS (integer vulnerability) annotation
//
class MEDS_LoopAnnotation : public MEDS_AnnotationBase
{
	public:
		MEDS_LoopAnnotation() : 
			loop_no(0), 
			preheader(0), 
			header(0)
			{};
		MEDS_LoopAnnotation(const string &p_rawLine);
		virtual ~MEDS_LoopAnnotation(){}

		virtual const string toString() const { return "loop annot "; }

	private:
		void init();
		void parse();

		string m_rawInputLine;

		uint64_t loop_no;
		IRDB_SDK::VirtualOffset_t preheader;
		IRDB_SDK::VirtualOffset_t header;
		set<IRDB_SDK::VirtualOffset_t> all_blocks;
		set<uint64_t> sub_loops;
};

}
#endif
