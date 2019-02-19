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
#include "MEDS_DeadRegAnnotation.hpp"
#include "MEDS_TakesAddressAnnotation.hpp"
#include "MEDS_IBAnnotation.hpp"
#include "MEDS_IBTAnnotation.hpp"
#include "MEDS_MemoryRangeAnnotation.hpp"

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



template <class type> bool MEDS_AnnotationParser::add_if_valid(string line) 
{
	type * annot=new type(line); 
	if (annot->isValid()) 
	{ 
		// note that this should _NOT_ be an if/else for every possible Annotation
		// this _should_ list the major categorizations of annotations
		// currently we have annotations that can be looked up by VirtualOffset and looked up by Function
		if(annot->isFuncAnnotation()) 
		{ 
			MEDS_FuncAnnotation* fannot=dynamic_cast<MEDS_FuncAnnotation*>(annot); 
			assert(fannot); 
			string nam=fannot->getFuncName(); 
			m_func_annotations.insert(MEDS_Annotations_FuncPair_t(nam, annot)); 
		} 
		else 
		{ 
			VirtualOffset vo = annot->getVirtualOffset(); 
			m_annotations.insert(MEDS_Annotations_Pair_t(vo, annot)); 
		} 
		return true;
	} 
	else 
	{ 
		delete annot; 
		return false;
	} 
}

void MEDS_AnnotationParser::parseFile(istream &p_inputStream)
{
	while (!p_inputStream.eof())
	{
		auto line=string();
		getline(p_inputStream, line);
		if (line.empty()) continue;

		if(add_if_valid<MEDS_DeadRegAnnotation>          (line)) continue;
		if(add_if_valid<MEDS_FPTRShadowAnnotation>       (line)) continue;
		if(add_if_valid<MEDS_InstructionCheckAnnotation> (line)) continue;
		if(add_if_valid<MEDS_FuncPrototypeAnnotation>    (line)) continue;
		if(add_if_valid<MEDS_SafeFuncAnnotation>         (line)) continue;
		if(add_if_valid<MEDS_ProblemFuncAnnotation>      (line)) continue;
		if(add_if_valid<MEDS_FRSafeAnnotation>           (line)) continue;
		if(add_if_valid<MEDS_FuncExitAnnotation>         (line)) continue;
		if(add_if_valid<MEDS_TakesAddressAnnotation>     (line)) continue;
		if(add_if_valid<MEDS_IBAnnotation>               (line)) continue;
		if(add_if_valid<MEDS_IBTAnnotation>              (line)) continue;
		if(add_if_valid<MEDS_MemoryRangeAnnotation>      (line)) continue;
	}
}

