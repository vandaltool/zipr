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

#include "MEDS_DeadRegAnnotation.hpp"

using namespace std;
using namespace MEDS_Annotation;

/* 
example:

            4010f0      4 INSTR DEADREGS  EFLAGS RAX ZZ sub     rsp, 8          ; _init

 need to parse out eflags, and rax into the regset variable.


*/

MEDS_DeadRegAnnotation::MEDS_DeadRegAnnotation() 
{
	setInvalid();	
}

MEDS_DeadRegAnnotation::MEDS_DeadRegAnnotation(const string &p_rawLine) 
{
	setInvalid();	
	m_rawInputLine=p_rawLine;
	parse();
}

void MEDS_DeadRegAnnotation::parse()
{
	string tofind=" INSTR DEADREGS ";
	size_t pos=m_rawInputLine.find(tofind);
	if (pos==string::npos)
		return;

        // get offset
        VirtualOffset vo(m_rawInputLine);
        m_virtualOffset = vo;

	cout <<"Found deadreg annotation in: "<<m_rawInputLine<<endl;
	// ignore result of getRegisterSet method because 
	// we don't need to parse the rest of the line.
	Register::readRegisterSet(m_rawInputLine.substr(pos+tofind.length()), regset);
	setValid();	
}

