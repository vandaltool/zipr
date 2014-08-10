#ifndef __ANNOTBOUNDGEN
#define __ANNOTBOUNDGEN
#include "PrecedenceBoundaryGenerator.hpp"
#include "MEDS_AnnotationParser.hpp"
#include "MEDS_AnnotationBase.hpp"
#include "MEDS_InstructionCheckAnnotation.hpp"
#include "VirtualOffset.hpp"

#include <fstream>

class AnnotationBoundaryGenerator : public PrecedenceBoundaryGenerator
{
protected:
	MEDS_Annotation::MEDS_AnnotationParser *annotParser;
public:
	AnnotationBoundaryGenerator(MEDS_Annotation::MEDS_AnnotationParser *annotParser) : annotParser(annotParser){}
	virtual std::vector<Range> GetBoundaries(libIRDB::Function_t *func);
};

#endif
