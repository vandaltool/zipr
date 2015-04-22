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

#include <iostream>
#include <cstdio>
#include <string>
#include <stdlib.h>
#include <string.h>

#include "MEDS_Register.hpp"
#include "MEDS_InstructionCheckAnnotation.hpp"

using namespace std;
using namespace MEDS_Annotation;

/*
Example format (as of 10/18/2011) -- subject to change:

80482bc 3 INSTR CHECK OVERFLOW UNSIGNED 32 EAX ZZ add eax, 1 
8048325 6 INSTR CHECK OVERFLOW SIGNED 16 [esp+2AH] ZZ add word ptr [esp+2Ah], 1 
804832b 6 INSTR CHECK OVERFLOW UNSIGNED 16 [esp+2CH] ZZ add word ptr [esp+2Ch], 1 
8048336 5 INSTR CHECK SIGNEDNESS SIGNED 16 AX ZZ mov [esp+28h], ax 
80483db 5 INSTR CHECK UNDERFLOW SIGNED 32 EAX ZZ sub eax, 7FFFFFFFh 
80483fd 3 INSTR CHECK UNDERFLOW SIGNED 32 EAX ZZ sub eax, 1 
8048492 5 INSTR CHECK TRUNCATION 32 EAX 16 AX ZZ mov [esp+26h], ax 
8048492 5 INSTR CHECK SIGNEDNESS SIGNED 16 AX ZZ mov [esp+26h], ax 
8048892 4 INSTR INFINITELOOP add     [ebp+var_25], 1
8048293 3 INSTR MEMSET STACKOFFSET 12 SIZE 24 ZZ call memset
*/

void MEDS_InstructionCheckAnnotation::init()
{
	m_isValid = false;
	m_isOverflow = false;
	m_isUnderflow = false;
	m_isTruncation = false;
	m_isSignedness = false;
	m_isSigned = false;
	m_isUnsigned = false;
	m_isUnknownSign = true;
	m_isNoFlag = false;
	m_isInfiniteLoop = false;
	m_isSevere = false;
	m_isMemset = false;
	m_flowsIntoCriticalSink = false;
	m_isIdiom = false;
	m_bitWidth = -1;
	m_truncationFromWidth = -1;
	m_truncationToWidth = -1;
	m_register = Register::UNKNOWN;
	m_register2 = Register::UNKNOWN;
	m_stackOffset = -1;
	m_objectSize = -1;
	m_isEspOffset = false;
	m_isEbpOffset = false;
	m_idiomNumber = -1;
}

MEDS_InstructionCheckAnnotation::MEDS_InstructionCheckAnnotation(const std::string &p_rawInputLine)
{
	init();
	m_rawInputLine = p_rawInputLine;
	parse();
}

MEDS_InstructionCheckAnnotation::MEDS_InstructionCheckAnnotation()
{
	init();
}

// parse and set all the member variables
void MEDS_InstructionCheckAnnotation::parse()
{

	// format:
	//	field 1 - instruction address
	//  field 2 - instruction size (ignore)
	//  field 3 - INSTR
	//  field 4 - CHECK | INFINITELOOP | MEMSET
	//  field 5 - {OVERFLOW | UNDERFLOW | SIGNEDNESS | TRUNCATION }  
	//  field 6 - {SIGNED | UNSIGNED | UNKNOWNSIGN | 16 | 32}
	//  field 7 - {<register> | <memory reference>}

	if (m_rawInputLine.find(MEDS_ANNOT_INSTR)==string::npos)
		return;

	if (m_rawInputLine.find(MEDS_ANNOT_INFINITELOOP)==string::npos &&
	    m_rawInputLine.find(MEDS_ANNOT_CHECK)==string::npos &&
	    m_rawInputLine.find(MEDS_ANNOT_MEMSET)==string::npos) 
	{
		return;
	}

	// get offset
	VirtualOffset vo(m_rawInputLine);
	m_virtualOffset = vo;

	// The annotation format is very simple so we don't bother with any fancy parsing
	//   8048913      3 INSTR CHECK OVERFLOW NOFLAGSIGNED 32 EDX+ECX ZZ lea     eax, [edx+ecx]
	// Later, this may need to be changed

	// get check type
	if (m_rawInputLine.find(MEDS_ANNOT_OVERFLOW)!=string::npos)
		m_isOverflow = true;

	if (m_rawInputLine.find(MEDS_ANNOT_UNDERFLOW)!=string::npos)
		m_isUnderflow = true;

	if (m_rawInputLine.find(MEDS_ANNOT_SIGNEDNESS)!=string::npos)
		m_isSignedness = true;
		
	if (m_rawInputLine.find(MEDS_ANNOT_TRUNCATION)!=string::npos)
		m_isTruncation = true;

	if (m_rawInputLine.find(MEDS_ANNOT_NOFLAG)!=string::npos)
		m_isNoFlag = true;

	if (m_rawInputLine.find(MEDS_ANNOT_MEMSET)!=string::npos)
		m_isMemset = true;

	if (m_rawInputLine.find(MEDS_ANNOT_FLOWS_INTO_CRITICAL_SINK)!=string::npos)
		m_flowsIntoCriticalSink = true;

	int idiom_pos = m_rawInputLine.find(MEDS_ANNOT_IDIOM);
	if (idiom_pos != string::npos)
	{
		idiom_pos += strlen(MEDS_ANNOT_IDIOM);
		setIdiomNumber(atoi(m_rawInputLine.substr(idiom_pos).c_str()));
		m_isIdiom = true;
	}

	// signed vs. unsigned
	if (m_rawInputLine.find(MEDS_ANNOT_UNSIGNED)!=string::npos)
	{
		m_isUnsigned = true;
		m_isUnknownSign = false;
	}
	else if (m_rawInputLine.find(MEDS_ANNOT_SIGNED)!=string::npos)
	{
		m_isSigned = true;
		m_isUnknownSign = false;
	}
	else if (m_rawInputLine.find(MEDS_ANNOT_UNKNOWNSIGN)!=string::npos)
	{
		m_isUnsigned = false;
		m_isSigned = false;
		m_isUnknownSign = true;
	}

    // check for infinite loop annotation
	// 8048565      3 INSTR INFINITELOOP
	if (m_rawInputLine.find(MEDS_ANNOT_INFINITELOOP)!=string::npos)
	{
		m_isInfiniteLoop = true;
	}

	// get bit width information for overflow & underflow
	if (m_isOverflow || m_isUnderflow)
	{
	// 8048565      6 INSTR CHECK OVERFLOW SIGNED 16  [ESP]+38 ZZ add     word ptr [esp+26h], 1
	// 804856b      6 INSTR CHECK OVERFLOW UNSIGNED 16  [ESP]+36 ZZ add     word ptr [esp+24h], 1
	// 80483bb      4 INSTR CHECK OVERFLOW UNKNOWNSIGN 16  AX ZZ add     ax, 7FBCh
	// 80483d5      3 INSTR CHECK UNDERFLOW SIGNED 16  CX ZZ sub     cx, ax
    // 804d51d      2 INSTR CHECK OVERFLOW UNSIGNED 32  EBX ZZ add     ebx, eax


		char buf[1024] = "";
		sscanf(m_rawInputLine.c_str(), "%*s %*d %*s %*s %*s %*s %d %s", &m_bitWidth, buf);
		m_target = string(buf);
		if (m_isNoFlag)
		{
			m_register = Register::getRegister(m_target);
		}
	}
	else if (m_isTruncation) // get bid width from/to information for truncation
	{
		char buf[1024] = "";
		char buf2[1024] = "";
		// [ADDR] [SIZE] INSTR CHECK TRUNCATION UNKNOWNSIGN 32 EAX 16 AX ZZ mov     [esp+2Ah], ax
		sscanf(m_rawInputLine.c_str(), "%*s %*d %*s %*s %*s %*s %d %s %d %s", &m_truncationFromWidth, buf, &m_truncationToWidth, buf2);

		m_target = string(buf);
		m_target2 = string(buf2);

		m_register = Register::getRegister(m_target);
		m_register2 = Register::getRegister(m_target2);

		// 20120410 STARS added SEVERE field to TRUNC annotations to specify that we must use a terminating/saturating policy
		if (m_rawInputLine.find(MEDS_ANNOT_SEVERE)!=string::npos)
		{
			m_isSevere = true;
		}
	} 
	else if (m_isSignedness)
	{
		char buf[1024] = "";
		// [ADDR] [SIZE] INSTR CHECK SIGNEDNESS SIGNED 16 AX ZZ mov     [esp+28h], ax
		// [ADDR] [SIZE] INSTR CHECK SIGNEDNESS UNSIGNED 16 AX ZZ mov   [esp+28h], ax
		sscanf(m_rawInputLine.c_str(), "%*s %*d %*s %*s %*s %*s %d %s", &m_bitWidth, buf);
		m_target = string(buf);
		m_register = Register::getRegister(m_target);
	}

	if (m_isMemset)
	{
		// 8048293 3 INSTR MEMSET STACKOFFSET_EBP 12 SIZE 24 ZZ call memset
		// 8048293 3 INSTR MEMSET STACKOFFSET_ESP 12 SIZE 24 ZZ call memset
		if (m_rawInputLine.find("STACKOFFSET")!=string::npos)
		{
			char buf[1024] = "";
			sscanf(m_rawInputLine.c_str(), "%*s %*d %*s %*s %*s %d %*s %d", &m_stackOffset, &m_objectSize);
			if (m_rawInputLine.find("STACKOFFSET_EBP")!=string::npos)
			{
				m_isEbpOffset = true;
			}
			else if (m_rawInputLine.find("STACKOFFSET_ESP")!=string::npos)
			{
				m_isEspOffset = true;
			}
		}
	}

	setValid(); // m_isValid = true;
}

