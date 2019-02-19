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

#ifndef _MEDS_SHADOWANNOTATION_H_
#define _MEDS_SHADOWANNOTATION_H_

#include <string>
#include "VirtualOffset.hpp"
#include "MEDS_AnnotationBase.hpp"

namespace MEDS_Annotation 
{

using namespace std;
using namespace MEDS_Annotation;

//
// Class to handle one MEDS shadow annotation
//
class MEDS_ShadowAnnotation : public MEDS_AnnotationBase
{
	public:
		MEDS_ShadowAnnotation() {
			m_shadowDefine = false;
			m_shadowCheck = false; 
			m_shadowId = -1; 
		};
		virtual ~MEDS_ShadowAnnotation() {}
	
		const bool isDefineShadowId() const { return m_shadowDefine; }
		const bool isCheckShadowId() const { return m_shadowCheck; }
		const int getShadowId() const { return m_shadowId; }

	protected:
		void setDefineShadowId() { m_shadowDefine = true; }
		void setCheckShadowId() { m_shadowCheck = true; }
		void setShadowId(int p_id) { m_shadowId = p_id; }

	private:
		bool m_shadowDefine;
		bool m_shadowCheck;
		int  m_shadowId;
};

}

#endif
