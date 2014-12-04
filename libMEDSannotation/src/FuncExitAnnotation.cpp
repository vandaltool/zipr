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

#include <iostream>
#include <cstdio>
#include <string>
#include <string.h>

#include "MEDS_Register.hpp"
#include "FuncExitAnnotation.hpp"

using namespace std;
using namespace MEDS_Annotation;





MEDS_FuncExitAnnotation::MEDS_FuncExitAnnotation(const string &p_rawLine)
{
	init();
	m_rawInputLine=p_rawLine;
	parse();
}

void MEDS_FuncExitAnnotation::init()
{
}


/*
	Example format -- subject to change:
   804925b      5 INSTR CALL TAILCALL jmp     check_one_fd


*/
void MEDS_FuncExitAnnotation::parse()
{

	if (m_rawInputLine.find(" INSTR ")==string::npos)
                return;

        if (
		m_rawInputLine.find(" INSTR RETURN ")==string::npos  &&
		m_rawInputLine.find(" INSTR CALL TAILCALL ")==string::npos 
	   )
	{
		/* INSTR  that's not for a safe fast return */
		return;
	}

        // get offset
        VirtualOffset vo(m_rawInputLine);
        m_virtualOffset = vo;

	setValid();	// no additional info recorded for right now.

//	if(getenv("VERBOSE")!=NULL)
		cout<<"Found TAILCALL annotation for "<<vo.to_string()<<endl;

}

