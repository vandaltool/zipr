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

#include "MEDS_FPTRShadowAnnotation.hpp"

using namespace std;
using namespace MEDS_Annotation;

/*
	Example format -- subject to change:

   804b3a6      5 INSTR FPTRSHADOW  [rsp+12] SHADOWID 7
   804c941      5 INSTR FPTRCHECK  [rsp+12] SHADOWID 7
   804c945      5 INSTR FPTRCHECK  [rsp] SHADOWID 8

   0x4006ca   7    INSTR FPTRSHADOW  {memory addressing expression} SHADOWID <id>

0x8098f00     e.g. INSTR FPTRSHADOW 0x8089f00  note absence of brackets

0x80487a0[RBX]
0x8047aa0[RBX+RCX]
0x8098f80[RAX+RDX*2] (NOTE: could be *4 or *8 instead of *2)
[RBX+RDX*4]
[RAX+RCX*8+12]
[RDX+RAX*2-64]
[R8+R15]
[R9+R11+48]
[R13+72]
[R12*4]
[R14*2-12]
*/

MEDS_FPTRShadowAnnotation::MEDS_FPTRShadowAnnotation() : MEDS_ShadowAnnotation()
{
	setInvalid();	
}

MEDS_FPTRShadowAnnotation::MEDS_FPTRShadowAnnotation(const string &p_rawLine) : MEDS_ShadowAnnotation()
{
	setInvalid();	
	m_rawInputLine=p_rawLine;
	parse();
}

void MEDS_FPTRShadowAnnotation::parse()
{
	if (m_rawInputLine.find(" INSTR ")==string::npos)
		return;

	if (m_rawInputLine.find(MEDS_ANNOT_FPTRSHADOW)!=string::npos)
		setDefineShadowId();

	if (m_rawInputLine.find(MEDS_ANNOT_FPTRCHECK)!=string::npos)
		setCheckShadowId();

	if (!isDefineShadowId() && !isCheckShadowId())
	{
		/* invalid annotation */
//		cerr<<"Found invalid FPTR SHADOW annotation: " << m_rawInputLine<<endl;
		setInvalid();	
		return;
	}

	// get offset
	VirtualOffset vo(m_rawInputLine);
	setVirtualOffset(vo);

	int shadowId;
	char buf[2048] = "";
	int instrSize;

	// 804b3a6      5 INSTR FPTRSHADOW  [esp+12] SHADOWID 7
    // 804c941      5 INSTR FPTRCHECK  [rdx] SHADOWID 7
	sscanf(m_rawInputLine.c_str(), "%*x %d %*s %*s %s SHADOWID %d", &instrSize, buf, &shadowId);

	setInstructionSize(instrSize);
	setExpression(std::string(buf));
	setShadowId(shadowId);

	cout << "virtual offset: " << hex << getVirtualOffset().getOffset() << dec << endl;
	cout << "size: " << getInstructionSize() << endl;
	cout << "expr: " << getExpression() << endl;
	cout << "shadow id: " << getShadowId() << endl;

	if (shadowId < 0)
	{
		setInvalid();
		cout << "invalid shadow id" << endl;
		return;
	}

	if (isCheckShadowId())
	{
		cout << "is check shadow id" << endl;
		if (!verifyCheckShadowExpression(getExpression()))
		{					
			setInvalid();
			cout << "invalid expression" << endl;
			return;
		}
	}

	cout << "valid annotation" << endl;
	setValid();	
}

bool MEDS_FPTRShadowAnnotation::verifyCheckShadowExpression(const string& expression)
{
	// register expression is of the form: [reg]
	string reg = m_expression.substr(1, expression.size()-2);
	return Register::isValidRegister(reg);
}

const Register::RegisterName MEDS_FPTRShadowAnnotation::getRegister() const
{
	// expected format; [reg]
	return Register::getRegister(m_expression.substr(1, m_expression.size()-2));
}

bool MEDS_FPTRShadowAnnotation::isRIPRelative() const
{
	return m_expression.find("rip") != string::npos;
}

// expect of the form:
//     [rip+constant]
//     [rip-constant]
uintptr_t MEDS_FPTRShadowAnnotation::computeRIPAddress()
{
	Register::RegisterName reg;
	int offset;
	uintptr_t instructionAddress;
	uintptr_t address;

	parseRegister(getExpression().c_str(), &reg, &offset);

	if (reg == Register::RIP) 
	{
		return getVirtualOffset().getOffset() + getInstructionSize() + offset;
	}
	else
		return 0;
}


int MEDS_FPTRShadowAnnotation::parseRegisterOffset(const char *p_buf)
{
	// e.g.: [rsp] [esp+12] [esp-12] 
	for (int i = 0; i < strlen(p_buf); ++i)
	{
		if (p_buf[i] == '-' || p_buf[i] == '+')
		{
			return atoi(&p_buf[i]);
		}
	}
	return 0;
}

// e.g.: [rsp] [esp+12] [esp-12] 
void MEDS_FPTRShadowAnnotation::parseRegister(const char *p_buf, Register::RegisterName *p_register, int *p_registerOffset)
{
	int startReg = -1;
	int endReg = -1;
	int signPos = -1;
	for (int i = 0; i < strlen(p_buf); ++i)
	{
		if (p_buf[i] == '[') startReg = i+1;
		if (p_buf[i] == ']') 
		{ 
			endReg = i-1; 
		}
		if (p_buf[i] == '-' || p_buf[i] == '+')
		{
			*p_registerOffset = atoi(&p_buf[i]);
			signPos = i;
		}
	}

cout << "analyzing: " << p_buf << endl;
	if (signPos >= 0)
		endReg = signPos - 1;
	if (startReg >= 0 && endReg >= startReg)
	{
		char registerBuf[1024];
		int size = endReg - startReg + 1;
		strncpy(registerBuf, &p_buf[startReg], size);
		registerBuf[size] = '\0';
		cout << "register buffer detected: " << registerBuf << endl;
		*p_register = Register::getRegister(registerBuf);
	}

	*p_registerOffset = parseRegisterOffset(p_buf);
}
