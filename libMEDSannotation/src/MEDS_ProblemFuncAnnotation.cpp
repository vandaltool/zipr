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

        if (m_rawInputLine.find("JUMPUNRESOLVED")!=string::npos)
	{
		if(getenv("VERBOSE"))
			cout<<"Found JUMPUNRESOLVED problem annotation for "<<getFuncName() << endl;
		markJumpUnresolved();	 
	}
        else if (m_rawInputLine.find("CALLUNRESOLVED")!=string::npos)
	{
		if(getenv("VERBOSE"))
			cout<<"Found CALLUNRESOLVED annotation for "<<getFuncName() << endl;
		markCallUnresolved();	 
	}

}

