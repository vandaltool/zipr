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

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <cstdio>
#include <string>

#include "MEDS_Register.hpp"
#include "MEDS_ProblemFuncAnnotation.hpp"

using namespace std;
using namespace MEDS_Annotation;





MEDS_ProblemFuncAnnotation::MEDS_ProblemFuncAnnotation(const string &p_rawLine)
{
	init();
	m_rawInputLine=p_rawLine;
	parse();
}

void MEDS_ProblemFuncAnnotation::init()
{
	MEDS_FuncAnnotation::init();
}


/*
	Example format (as of August 15, 2014 ) -- subject to change:

    400420      6 FUNC PROBLEM .__libc_start_main JUMPUNRESOLVED
    400430      6 FUNC PROBLEM .__printf_chk CHUNKS JUMPUNRESOLVED
    400490    100 FUNC PROBLEM __do_global_dtors_aux CALLUNRESOLVED


*/
void MEDS_ProblemFuncAnnotation::parse()
{

	if (m_rawInputLine.find("FUNC ")==string::npos)
                return;

        size_t pos=m_rawInputLine.find("FUNC PROBLEM");
        if ( pos==string::npos )
	{
		/* FUNC line that's not local or global?  I'm confused. */
		/* could be a FUNC GLOBAL, etc. line */
		return;
	}

	size_t func_name_start_pos=pos+strlen("FUNC PROBLEM ");
	size_t func_end_pos=m_rawInputLine.find(" ", func_name_start_pos);
	assert(func_end_pos!=string::npos);
	string func_name=m_rawInputLine.substr(func_name_start_pos, func_end_pos-func_name_start_pos);

        // get offset
	setFuncName(func_name);
	cout<<"Found problem func name='"<<func_name<<"'"<<endl;

	if(!isValid()) matches(m_rawInputLine,"JUMPUNRSOLVED", pt_JumpUnresolved);
	if(!isValid()) matches(m_rawInputLine,"CALLUNRSOLVED", pt_CallUnresolved);
	if(!isValid()) matches(m_rawInputLine,"STACKANALYSIS", pt_BadStackAnalysis);
	if(!isValid()) matches(m_rawInputLine,"BADRTLS", pt_BadRTLs);
	if(!isValid()) 
	{
		pt=pt_Unknown;
		setValid();
	}
	
}

bool MEDS_ProblemFuncAnnotation::matches(string line, string pattern, ProblemType prob_type)
{
	
        if (line.find(pattern)!=string::npos)
	{
		if(getenv("VERBOSE"))
			cout<<"Found "<<pattern<<" problem annotation for "<<getFuncName() << endl;
		pt=prob_type;
		setValid();
	}
}
