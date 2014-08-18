#include <string>
#include <fstream>

#include "MEDS_AnnotationParser.hpp"
#include "MEDS_InstructionCheckAnnotation.hpp"
#include "MEDS_SafeFuncAnnotation.hpp"
#include "MEDS_ProblemFuncAnnotation.hpp"

// @todo: multiple annotation per instruction

using namespace std;
using namespace MEDS_Annotation;

MEDS_AnnotationParser::MEDS_AnnotationParser(istream &p_inputStream)
{
	parseFile(p_inputStream);
}

void MEDS_AnnotationParser::parseFile(const std::string &input_filename)
{
	ifstream infile(input_filename.c_str(), ifstream::in);

	if(!infile.is_open())
		throw string("File not found");

	parseFile(infile);
}

void MEDS_AnnotationParser::parseFile(istream &p_inputStream)
{
	string line;

	while (!p_inputStream.eof())
	{
	    	getline(p_inputStream, line);
		if (line.empty()) continue;


#define 	ADD_AND_CONTINUE_IF_VALID(type) \
		{ \
			type * annot=new type(line); \
			if (annot->isValid()) \
			{ \
				if(annot->isFuncAnnotation()) \
				{ \
					MEDS_FuncAnnotation* fannot=dynamic_cast<MEDS_FuncAnnotation*>(annot); \
					assert(fannot); \
					string nam=fannot->getFuncName(); \
					m_func_annotations.insert(MEDS_Annotations_FuncPair_t(nam, annot)); \
				} \
				else \
				{ \
					VirtualOffset vo = annot->getVirtualOffset(); \
					m_annotations.insert(MEDS_Annotations_Pair_t(vo, annot)); \
				} \
				continue; \
			} \
			else \
				delete annot; \
		}

		ADD_AND_CONTINUE_IF_VALID(MEDS_InstructionCheckAnnotation);
		ADD_AND_CONTINUE_IF_VALID(MEDS_SafeFuncAnnotation);
		ADD_AND_CONTINUE_IF_VALID(MEDS_ProblemFuncAnnotation);

//				cout<<"Found annotation: "<<annot->toString()<<endl;\
		
	}
}

