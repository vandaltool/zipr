/*
 * Copyright (c) 2015 - Zephyr Software LLC
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software LLC
 * URL   : http://www.zephyr-software.com/
 */

#ifndef _LIBTRANSFORM_POINTERCHECK64_H_
#define _LIBTRANSFORM_POINTERCHECK64_H_

#include "integertransform64.hpp"

namespace libTransform
{

using namespace std;
using namespace libIRDB;

class PointerCheck64 : public IntegerTransform64
{
	public:
		PointerCheck64(VariantID_t *, FileIR_t*, MEDS_Annotations_t *p_annotations, set<std::string> *p_filteredFunctions, set<VirtualOffset> *p_warnings); 

		virtual int execute();
		virtual Instruction_t* addCallbackHandlerSequence(Instruction_t *p_orig, Instruction_t *p_fallthrough, std::string p_detector, int p_policy);

};

} // end namespace

#endif
