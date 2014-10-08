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

