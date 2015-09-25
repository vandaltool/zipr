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

#ifndef _MEDS_ANNOTATION_PARSER_H_
#define _MEDS_ANNOTATION_PARSER_H_

#include <map>
#include <iostream>

#include "MEDS_AnnotationBase.hpp"
#include "VirtualOffset.hpp"

namespace MEDS_Annotation
{

typedef std::multimap<const VirtualOffset, MEDS_AnnotationBase*> MEDS_Annotations_t;
typedef std::pair<const VirtualOffset, MEDS_AnnotationBase*> MEDS_Annotations_Pair_t;

typedef std::multimap<const std::string, MEDS_AnnotationBase*> MEDS_FuncAnnotations_t;
typedef std::pair<const std::string, MEDS_AnnotationBase*> MEDS_Annotations_FuncPair_t;

class MEDS_AnnotationParser
{
	public:
		MEDS_AnnotationParser() {};
		MEDS_AnnotationParser(std::istream &); 	/* pass opened file */
		void parseFile(std::istream &);
		void parseFile(const std::string &);	 /* pass filename */
		MEDS_Annotations_t &         getAnnotations() { return m_annotations; }
		MEDS_FuncAnnotations_t & getFuncAnnotations() { return m_func_annotations; }
		//MEDS_Annotations_t & getFuncPrototypeAnnotations() { return m_func_prototype_annotations; }

	private:
		MEDS_Annotations_t m_annotations;
		MEDS_FuncAnnotations_t m_func_annotations;
		//MEDS_Annotations_t m_func_prototype_annotations;
};

}

#endif
