#include <string>
#include "MEDS_AnnotationParser.hpp"

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

		MEDS_InstructionCheckAnnotation annot(line);

		if (annot.isValid())
		{
			VirtualOffset vo = annot.getVirtualOffset();
//			m_annotations[vo] = annot;
//			m_annotations.insert(vo, annot);
			m_annotations.insert(std::pair<VirtualOffset, MEDS_InstructionCheckAnnotation>(vo, annot));
		}
	}
}

