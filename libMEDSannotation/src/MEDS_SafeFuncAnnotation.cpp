#include <stdlib.h>

#include <iostream>
#include <cstdio>
#include <string>

#include "MEDS_Register.hpp"
#include "MEDS_SafeFuncAnnotation.hpp"

using namespace std;
using namespace MEDS_Annotation;





MEDS_SafeFuncAnnotation::MEDS_SafeFuncAnnotation(const string &p_rawLine)
{
	init();
	m_rawInputLine=p_rawLine;
	parse();
}

void MEDS_SafeFuncAnnotation::init()
{
	MEDS_AnnotationBase::init();
}


/*
	Example format (as of July 31, 2014 ) -- subject to change:

    400448     24 FUNC GLOBAL .init_proc FUNC_UNSAFE NOFP RET     40045f
    400470      6 FUNC GLOBAL .__stack_chk_fail FUNC_UNSAFE NOFP NORET FUNC_LEAF     400475
    400480      6 FUNC GLOBAL .__libc_start_main FUNC_UNSAFE NOFP RET FUNC_LEAF     400485
    400490      6 FUNC GLOBAL .__printf_chk FUNC_UNSAFE NOFP RET FUNC_LEAF     400495

*/
void MEDS_SafeFuncAnnotation::parse()
{

	if (m_rawInputLine.find("FUNC ")==string::npos)
                return;

        if (m_rawInputLine.find("FUNC GLOBAL")==string::npos &&
            m_rawInputLine.find("FUNC LOCAL")==string::npos )
	{
		/* FUNC line that's not local or global?  I'm confused. */
		/* could be a FUNC PROBLEM line */
		return;
	}

        // get offset
        VirtualOffset vo(m_rawInputLine);
        m_virtualOffset = vo;

        if (m_rawInputLine.find(" FUNC_SAFE ")!=string::npos)
	{
		if(getenv("VERBOSE"))
			cout<<"Found FUNC_SAFE annotation for "<<vo.to_string()<<endl;
		markSafe();	 // sets valid
	}
        else if (m_rawInputLine.find(" FUNC_UNSAFE ")!=string::npos)
	{
		if(getenv("VERBOSE"))
			cout<<"Found FUNC_UNSAFE annotation for "<<vo.to_string()<<endl;
		markUnsafe();	 // sets valid
	}

}

