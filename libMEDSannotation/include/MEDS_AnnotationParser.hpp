#ifndef _MEDS_ANNOTATION_PARSER_H_
#define _MEDS_ANNOTATION_PARSER_H_

#include <map>
#include <iostream>

#include "MEDS_AnnotationBase.hpp"
#include "VirtualOffset.hpp"

namespace MEDS_Annotation
{

typedef std::multimap<VirtualOffset, MEDS_AnnotationBase> MEDS_Annotations_t;

class MEDS_AnnotationParser
{
	public:
		MEDS_AnnotationParser(std::istream &);
		// std::multimap<VirtualOffset, MEDS_Annotation::MEDS_AnnotationBase> getAnnotations() { return m_annotations; }
		MEDS_Annotations_t getAnnotations() { return m_annotations; }

	private:
		//std::multimap<VirtualOffset, MEDS_Annotation::MEDS_AnnotationBase> 
		MEDS_Annotations_t m_annotations;
};

}

#endif
