#ifndef _MEDS_ANNOTATION_PARSER_H_
#define _MEDS_ANNOTATION_PARSER_H_

#include <map>
#include <iostream>

#include "MEDS_AnnotationBase.hpp"
#include "VirtualOffset.hpp"

namespace MEDS_Annotation
{

typedef std::multimap<VirtualOffset, MEDS_AnnotationBase*> MEDS_Annotations_t;
typedef std::pair<VirtualOffset, MEDS_AnnotationBase*> MEDS_Annotations_Pair_t;

typedef std::multimap<std::string, MEDS_AnnotationBase*> MEDS_FuncAnnotations_t;
typedef std::pair<std::string, MEDS_AnnotationBase*> MEDS_Annotations_FuncPair_t;

class MEDS_AnnotationParser
{
	public:
		MEDS_AnnotationParser() {};
		MEDS_AnnotationParser(std::istream &); 	/* pass opened file */
		void parseFile(std::istream &);
		void parseFile(const std::string &);	 /* pass filename */
		MEDS_Annotations_t &         getAnnotations() { return m_annotations; }
		MEDS_FuncAnnotations_t & getFuncAnnotations() { return m_func_annotations; }

	private:
		MEDS_Annotations_t m_annotations;
		MEDS_FuncAnnotations_t m_func_annotations;
};

}

#endif
