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

#ifndef MEDS_ANNOTATION_BASE_HPP
#define MEDS_ANNOTATION_BASE_HPP


#include <assert.h>
#include "VirtualOffset.hpp"


namespace MEDS_Annotation
{


// base class for deriving objects.
class MEDS_AnnotationBase
{
	public:			
		MEDS_AnnotationBase() { init(); }
		virtual ~MEDS_AnnotationBase()  { }

		/* i'd rather make this a pure virtual func, but can't figure out why it won't compile */
		virtual const std::string toString() const { assert(0); }

		// valid annotation?
		virtual bool isValid() const { return m_isValid; }
		virtual void setValid() { m_isValid = true; }
		virtual void setInvalid() { m_isValid = false; }

		// virtual offset
		virtual VirtualOffset getVirtualOffset() const { return m_virtualOffset; }
		virtual void setVirtualOffset(VirtualOffset p_vo) { m_virtualOffset = p_vo; }

		virtual void setInstructionSize(int p_size) { m_size = p_size; }
		virtual int getInstructionSize() { return m_size; }

		virtual void init() { m_isValid = false; m_size = 0; }

		virtual bool isFuncAnnotation() const { return false; } // false by default 
		virtual bool isFuncPrototypeAnnotation() const { return false; } // false by default 

	protected:
		bool m_isValid;
		int m_size;
		VirtualOffset  m_virtualOffset;
};

}

#endif
