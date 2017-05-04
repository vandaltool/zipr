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

           805829d      3 INSTR FPTRCHECK  [EBP-40] SHADOWID 5
           8057fa7      3 INSTR FPTRSHADOW  [EAX+8] SHADOWID 5
           80822aa      7 INSTR FPTRSHADOW  0 SHADOWID 6
           80822cc      3 INSTR FPTRSHADOW  EAX SHADOWID 6
           480faa      9 INSTR FPTRSHADOW  4721886 SHADOWID 75
*/

MEDS_FPTRShadowAnnotation::MEDS_FPTRShadowAnnotation() : MEDS_ShadowAnnotation()
{
	setInvalid();	
	setCriticalArgumentShadow(false);
	setFunctionPointerShadow(false);
}

MEDS_FPTRShadowAnnotation::MEDS_FPTRShadowAnnotation(const string &p_rawLine) : MEDS_ShadowAnnotation()
{
	setInvalid();	
	setCriticalArgumentShadow(false);
	setFunctionPointerShadow(false);
	m_rawInputLine=p_rawLine;
	parse();
}

void MEDS_FPTRShadowAnnotation::parse()
{
	if (m_rawInputLine.find(" INSTR ")==string::npos)
		return;

	if (m_rawInputLine.find(MEDS_ANNOT_FPTRSHADOW)!=string::npos)
	{
		setFunctionPointerShadow(true);
		setDefineShadowId();
	}

	if (m_rawInputLine.find(MEDS_ANNOT_ARGSHADOW)!=string::npos)
	{
		setCriticalArgumentShadow(true);
		setDefineShadowId();
	}

	if (m_rawInputLine.find(MEDS_ANNOT_FPTRCHECK)!=string::npos)
	{
		setFunctionPointerShadow(true);
		setCheckShadowId();
	}

	if (m_rawInputLine.find(MEDS_ANNOT_ARGCHECK)!=string::npos)
	{
		setCriticalArgumentShadow(true);
		setCheckShadowId();
	}

	if (!isDefineShadowId() && !isCheckShadowId())
	{
		/* invalid annotation */
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

// 8057fa7      3 INSTR FPTRSHADOW  [EAX+8] SHADOWID 5
// 80822aa      7 INSTR FPTRSHADOW  0 SHADOWID 6
// 80822cc      3 INSTR FPTRSHADOW  EAX SHADOWID 6
// 8480faa      9 INSTR FPTRSHADOW  4721886 SHADOWID 75
bool MEDS_FPTRShadowAnnotation::verifyCheckShadowExpression(const string& expression)
{
	return isMemoryExpression() || isRegister() || isConstant();
}

//           80822aa      7 INSTR FPTRSHADOW  0 SHADOWID 6
bool MEDS_FPTRShadowAnnotation::isConstant() const
{
	if (isRegister() || isMemoryExpression()) return false;

	long long val = 0;
	std::stringstream ss(getExpression());
	ss >> val;
	return !ss.bad();
}

long long MEDS_FPTRShadowAnnotation::getConstantValue(bool &p_valid) const
{
	p_valid = false;
	if (!isConstant())
		return 0;

	long long val = 0;
	std::stringstream ss(getExpression());
	ss >> val;
	p_valid = !ss.bad();
	return val;
}

// 80822cc      3 INSTR FPTRSHADOW  EAX SHADOWID 6
bool MEDS_FPTRShadowAnnotation::isRegister() const
{
	return Register::getRegister(getExpression()) != rn_UNKNOWN;
}

// 805829d      3 INSTR FPTRCHECK  [EBP-40] SHADOWID 5
bool MEDS_FPTRShadowAnnotation::isMemoryExpression() const
{
	return (m_expression.find('[') != std::string::npos &&
	    m_expression.find(']') != std::string::npos);
}

const RegisterName MEDS_FPTRShadowAnnotation::getRegister() const
{
	if (isRegister())
	{
		return Register::getRegister(getExpression());
	}
	else
	{
		return rn_UNKNOWN;
	}
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
	RegisterName reg;
	int offset;
	uintptr_t instructionAddress;
	uintptr_t address;

	parseRegister(getExpression().c_str(), &reg, &offset);

	if (reg == rn_RIP) 
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
void MEDS_FPTRShadowAnnotation::parseRegister(const char *p_buf, RegisterName *p_register, int *p_registerOffset)
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
