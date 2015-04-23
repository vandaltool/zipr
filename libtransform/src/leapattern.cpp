/*
 * Copyright (c) 2013, 2014 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sstream>

#include "leapattern.hpp"

using namespace std;
using namespace libTransform;

//
// Supported patterns:
//     reg+reg
//     reg+k
//     reg*reg
//     reg*k
//
// Not (yet) supported:
//     reg+reg+k
//
LEAPattern::LEAPattern(const MEDS_InstructionCheckAnnotation& p_annotation)
{
	m_isValid = false;
	m_isRegPlusReg = false;
	m_isRegPlusConstant = false;
	m_isRegTimesConstant = false;
	m_isRegTimesReg = false;
	m_reg1 = Register::UNKNOWN;
	m_reg2 = Register::UNKNOWN;
	m_constant = 0;

    if(regcomp(&m_regex_reg_plus_reg ,"[eax|ebx|ecx|edx|esi|edi|ebp|rax|rbx|rcx|rdx|rbp|rsi|rdi|r8|r9|r10|r11|r12|r13|r14|r15]\\+[eax|ebx|ecx|edx|esi|edi|ebp|rax|rbx|rcx|rdx|rbp|rsi|rdi|r8|r9|r10|r11|r12|r13|r14|r15]", REG_EXTENDED | REG_ICASE) !=0)
	    cerr << "Error: regular expression for reg+reg failed" << endl;

    if(regcomp(&m_regex_reg_times_reg ,"[eax|ebx|ecx|edx|esi|edi|ebp|rax|rbx|rcx|rdx|rbp|rsi|rdi|r8|r9|r10|r11|r12|r13|r14|r15]\\*[eax|ebx|ecx|edx|esi|edi|ebp|rax|rbx|rcx|rdx|rbp|rsi|rdi|r8|r9|r10|r11|r12|r13|r14|r15]", REG_EXTENDED | REG_ICASE) !=0)
	    cerr << "Error: regular expression for reg*reg failed" << endl;

    if(regcomp(&m_regex_reg_plus_constant ,"[eax|ebx|ecx|edx|esi|edi|ebp|rax|rbx|rcx|rdx|rbp|rsi|rdi|r8|r9|r10|r11|r12|r13|r14|r15]\\+[0-9+]", REG_EXTENDED | REG_ICASE) !=0)
	    cerr << "Error: regular expression for reg+constant failed" << endl;

    if(regcomp(&m_regex_reg_plus_negconstant ,"[eax|ebx|ecx|edx|esi|edi|ebp|rax|rbx|rcx|rdx|rbp|rsi|rdi|r8|r9|r10|r11|r12|r13|r14|r15]\\+\\-[0-9+]", REG_EXTENDED | REG_ICASE) !=0)
	    cerr << "Error: regular expression for reg+constant failed" << endl;

    if(regcomp(&m_regex_reg_times_constant ,"[eax|ebx|ecx|edx|esi|edi|ebp|rax|rbx|rcx|rdx|rbp|rsi|rdi|r8|r9|r10|r11|r12|r13|r14|r15]\\*[0-9+]", REG_EXTENDED | REG_ICASE) !=0)
	    cerr << "Error: regular expression for reg+constant failed" << endl;

	const int max = 5;
	regmatch_t pmatch[max];
	int countMatch = 0;

	if(regexec(&m_regex_reg_plus_reg, p_annotation.getTarget().c_str(), max, pmatch, 0)==0)
	{
		// pattern is of the form reg+reg, e.g.:   EDX+EAX
		//                                         r8+rbp
		//                                         r8+r9
		//                                         rax+r9
		string r1, r2;
		extractOperands(p_annotation.getTarget(), r1, r2);
		cerr << "leapattern: extract ops: " << r1 << " " << r2 << endl;
		m_reg1 = Register::getRegister(r1);
		m_reg2 = Register::getRegister(r2);

		if (m_reg1 != Register::UNKNOWN && m_reg2 != Register::UNKNOWN)
		{
			countMatch++;
			m_isRegPlusReg = true;
			m_isValid = true;
			cerr << "leapattern: reg+reg:" << Register::toString(m_reg1) << " " << Register::toString(m_reg2) << endl;  
		}
	}
    
	if(regexec(&m_regex_reg_times_reg, p_annotation.getTarget().c_str(), max, pmatch, 0)==0)
	{
		// pattern is of the form reg*reg, e.g.:   EDX*EAX
		// nb: is this even a valid pattern -- not used 
		string r1, r2;
		extractOperands(p_annotation.getTarget(), r1, r2);
		m_reg1 = Register::getRegister(r1);
		m_reg2 = Register::getRegister(r2);
		cerr << "leapattern: extract ops: " << r1 << " " << r2 << endl;

		if (m_reg1 != Register::UNKNOWN && m_reg2 != Register::UNKNOWN)
		{
			countMatch++;
			m_isRegTimesReg = true;
			m_isValid = true;
			cerr << "leapattern: reg*reg:" << Register::toString(m_reg1) << " " << Register::toString(m_reg2) << endl;  
		}
	}
	
// integertransform: reg+constant: register: EDX constant: 80 target register: EAX  annotation:    805525b      6 INSTR CHECK OVERFLOW NOFLAGUNSIGNED 32 EDX+128 ZZ lea     eax, [edx+80h]

	if(regexec(&m_regex_reg_plus_constant, p_annotation.getTarget().c_str(), max, pmatch, 0)==0 ||
	        regexec(&m_regex_reg_plus_negconstant, p_annotation.getTarget().c_str(), max, pmatch, 0)==0)
	{
		// pattern is of the form: reg+constant, e.g.: EDX+16
		// pattern is of the form: reg+-constant, e.g.: EAX+-16
		// note that constant value of annotation is in decimal (not hex)
		string r1, k;
		extractOperands(p_annotation.getTarget(), r1, k);
		m_reg1 = Register::getRegister(r1);
		cerr << "leapattern: extract ops: " << r1 << " " << k << endl;

		stringstream constantSS(k);
		if (constantSS >> m_constant)
		{
			m_isRegPlusConstant = true;
			m_isValid = true;
			countMatch++;
			cerr << "leapattern: reg+-constant: stream: " << Register::toString(m_reg1) << " constant: " << dec << m_constant << " annotation: " << p_annotation.toString() << endl;
		}
	}
	
	if(regexec(&m_regex_reg_times_constant, p_annotation.getTarget().c_str(), max, pmatch, 0)==0)
	{
		// pattern is of the form: reg*constant, e.g.: EDX*4
		// note that constant value of annotation is in decimal (not hex)
		string r1, k;
		extractOperands(p_annotation.getTarget(), r1, k);
		cerr << "leapattern: extract ops: " << r1 << " " << k << endl;
		m_reg1 = Register::getRegister(r1);
		stringstream constantSS(k);
		if (constantSS >> m_constant)
		{
			countMatch++;
			cerr << "leapattern: reg*constant: stream: " << p_annotation.getTarget().substr(4) << " constant: " << dec << m_constant << " annotation: " << p_annotation.toString() << endl;
			m_isValid = true;
			m_isRegTimesConstant = true;
		}
	}

	if (countMatch == 0 || countMatch > 1)
	{
		cerr << "leapattern: matching patterns = " << countMatch << " : " << p_annotation.toString() << endl;
		m_isValid = false;
	}
}

bool LEAPattern::isValid() const
{
	return m_isValid;
}

bool LEAPattern::isRegisterPlusRegister() const
{
	return m_isRegPlusReg;
}

bool LEAPattern::isRegisterTimesRegister() const
{
	return m_isRegTimesReg;
}

bool LEAPattern::isRegisterPlusConstant() const
{
	return m_isRegPlusConstant;
}

bool LEAPattern::isRegisterTimesConstant() const
{
	return m_isRegTimesConstant;
}

Register::RegisterName LEAPattern::getRegister1() const
{
	return m_reg1;
}

Register::RegisterName LEAPattern::getRegister2() const
{
	return m_reg2;
}

int LEAPattern::getConstant() const
{
	return m_constant;
}

// assume is of the form:
//       <operand><sign>[minus]<operand>
// where <operand> is a register or constant,
//       <sign> is * or +
void LEAPattern::extractOperands(const string target, string& op1, string& op2)
{
	int multiplyPos = target.find('*');
	int plusPos = target.find('+');
	int minusPos = target.find('-');

	int signPos = -1;
	if (multiplyPos >= 0) signPos = multiplyPos;
	if (plusPos >= 0) signPos = plusPos;

	op1 = op2 = "";

	if (signPos < 0) return;

	int op1len = signPos;
	int op2len;
	
	if (minusPos >= 0) 
			op2len = target.size() - minusPos - 1;
	else
			op2len = target.size() - signPos - 1;

	op1 = target.substr(0, op1len);

	if (minusPos >= 0) 
		op2 = target.substr(signPos+2, op2len); // e.g., rax+-5
	else
		op2 = target.substr(signPos+1, op2len);
}
