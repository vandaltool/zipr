#include <stdlib.h>

#include <iostream>
#include <cstdio>
#include <string>
#include <string.h>

#include "MEDS_Register.hpp"
#include "MEDS_FRSafeAnnotation.hpp"

using namespace std;
using namespace MEDS_Annotation;





MEDS_FRSafeAnnotation::MEDS_FRSafeAnnotation(const string &p_rawLine)
{
	init();
	m_rawInputLine=p_rawLine;
	parse();
}

void MEDS_FRSafeAnnotation::init()
{
}


/*
	Example format (as of July 31, 2014 ) -- subject to change:

   804b3a6      5 INSTR CALL NOFASTRETURN RAUNSAFE ZZ call frame_dummy 
   804c941      5 INSTR CALL FASTRETURN ZZ call    _ZN7GString3cmpEPKc; GString::cmp(char const*) 
   804c511      3 INSTR INDIRCALL NOFASTRETURN INDIRECT ZZ call    dword ptr [eax+8] 
   804c6fa      1 INSTR RETURN NOFASTRETURN NOCALLERS ZZ retn 

*/
void MEDS_FRSafeAnnotation::parse()
{

	if (m_rawInputLine.find(" INSTR ")==string::npos)
                return;

        if (
		m_rawInputLine.find(" INSTR CALL FASTRETURN ")==string::npos &&
		m_rawInputLine.find(" INSTR INDCALL FASTRETURN ")==string::npos &&
		m_rawInputLine.find(" INSTR RETURN FASTRETURN ")==string::npos 
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
		cout<<"Found FASTRETURN annotation for "<<vo.to_string()<<endl;

}

