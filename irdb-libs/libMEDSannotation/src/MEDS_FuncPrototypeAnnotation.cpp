/*
 * Copyright (c) 2014, 2015 - Zephyr Software LLC
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
#include "MEDS_FuncPrototypeAnnotation.hpp"

using namespace std;
using namespace MEDS_Annotation;

MEDS_FuncPrototypeAnnotation::MEDS_FuncPrototypeAnnotation(const string &p_rawLine)
{
	init();
	m_rawInputLine=p_rawLine;
	parse();
}

void MEDS_FuncPrototypeAnnotation::init()
{
	MEDS_AnnotationBase::init();
	m_returnArg = NULL; 
	m_arguments = NULL;
}


/*
4046e0     71 FUNC INARGS    4  ARG0 1 ARG1 0 ARG2 0 ARG3 0
404740    697 FUNC RETURNTYPE RAX 1
*/
void MEDS_FuncPrototypeAnnotation::parse()
{
	bool about_inargs = false;
	bool about_return = false;

	if (m_rawInputLine.find("FUNC ")==string::npos)
		return;

	if (m_rawInputLine.find("INARGS")!=string::npos)
	{
		about_inargs = true;
	}

	if (m_rawInputLine.find("RETURNTYPE")!=string::npos)
	{
		about_return = true;
	}

	if (!about_inargs && !about_return)
		return;

	// get offset
	VirtualOffset vo(m_rawInputLine);
	m_virtualOffset = vo;

	const int maxbufsize = m_rawInputLine.size()*2;

	if (about_inargs)
	{
// 4046e0     71 FUNC INARGS    4  ARG0 1 ARG1 0 ARG2 0 ARG3 0
		int numargs = 0;
		char buf[maxbufsize]; bzero(buf, maxbufsize);
		strncpy(buf, m_rawInputLine.c_str(), maxbufsize-1);
		buf[maxbufsize-1] = '\0';
		sscanf(buf, "%*x %*d %*s %*s %d %*s", &numargs);
		for (int i = 0; i < numargs; ++i)
		{
			char arg[24];
			sprintf(arg, "ARG%d", i);			
			char *zarg = strstr(buf, arg);
			if (zarg)
			{
				int meds_type;
				sscanf(zarg,"%*s %d", &meds_type);
				MEDS_Arg marg(meds_type);
				addArg(marg);
			}
			else
			{
				setInvalid();
				return;
			}
		}
	}
	else if (about_return)
	{
		// 404740    697 FUNC RETURNTYPE RAX 1
		char regbuf[maxbufsize]; bzero(regbuf, maxbufsize);
		int meds_retType;
		sscanf(m_rawInputLine.c_str(), "%*x %*d %*s %*s %s %d", regbuf, &meds_retType);
		RegisterName reg = Register::getRegister(regbuf);
		MEDS_Arg marg(meds_retType, reg);
		setReturnArg(marg);
//		cout << "   adding return argument: " << Register::toString(reg) << " type: " << meds_retType << endl;
	}
	else
	{
		assert(0);
		return;
	}			

	setValid();
}

