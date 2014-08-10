#include <string>

#include "MEDS_AnnotationParser.hpp"
#include "MEDS_InstructionCheckAnnotation.hpp"
#include "MEDS_SafeFuncAnnotation.hpp"

// @todo: multiple annotation per instruction

using namespace std;
using namespace MEDS_Annotation;

MEDS_AnnotationParser::MEDS_AnnotationParser(istream &p_inputStream)
{
	string line;

	while (!p_inputStream.eof())
	{
	    	getline(p_inputStream, line);
		if (line.empty()) continue;


#define 	ADD_AND_CONTINUE_IF_VALID(type) \
		{ \
			type annot(line); \
			if (annot.isValid()) \
			{ \
				VirtualOffset vo = annot.getVirtualOffset(); \
				m_annotations.insert(std::pair<VirtualOffset, MEDS_AnnotationBase>(vo, annot)); \
				continue; \
			} \
		}

		ADD_AND_CONTINUE_IF_VALID(MEDS_InstructionCheckAnnotation);
		ADD_AND_CONTINUE_IF_VALID(MEDS_SafeFuncAnnotation);
		
	}
}

