#ifndef _MEDS_ANNOTATION_PARSER_H_
#define _MEDS_ANNOTATION_PARSER_H_

#include <map>
#include <iostream>

#include "MEDS_InstructionCheckAnnotation.hpp"
#include "VirtualOffset.hpp"

namespace MEDS_Annotation
{

class MEDS_AnnotationParser
{
	public:
		MEDS_AnnotationParser(std::istream &);
		std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> getAnnotations() { return m_annotations; }

	private:
		std::map<VirtualOffset, MEDS_InstructionCheckAnnotation> m_annotations;
};

}

#endif
