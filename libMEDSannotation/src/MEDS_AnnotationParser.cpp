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

#include <string>
#include <fstream>

#include "MEDS_AnnotationParser.hpp"
#include "MEDS_InstructionCheckAnnotation.hpp"
#include "FuncExitAnnotation.hpp"
#include "MEDS_FuncPrototypeAnnotation.hpp"
#include "MEDS_SafeFuncAnnotation.hpp"
#include "MEDS_ProblemFuncAnnotation.hpp"
#include "MEDS_FRSafeAnnotation.hpp"
#include "MEDS_FPTRShadowAnnotation.hpp"

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

//cerr << "MEDS_AnnotationParser: line: " << line << endl;

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
			{ \
				delete annot; \
			} \
		}

		ADD_AND_CONTINUE_IF_VALID(MEDS_FPTRShadowAnnotation);
		ADD_AND_CONTINUE_IF_VALID(MEDS_InstructionCheckAnnotation);
		ADD_AND_CONTINUE_IF_VALID(MEDS_FuncPrototypeAnnotation);
		ADD_AND_CONTINUE_IF_VALID(MEDS_SafeFuncAnnotation);
		ADD_AND_CONTINUE_IF_VALID(MEDS_ProblemFuncAnnotation);
		ADD_AND_CONTINUE_IF_VALID(MEDS_FRSafeAnnotation);
		ADD_AND_CONTINUE_IF_VALID(MEDS_FuncExitAnnotation);

//				cout<<"Found annotation: "<<annot->toString()<<endl;\
		
	}
}

