#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sstream>

#include "leapattern.hpp"

using namespace std;
using namespace libTransform;

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
		countMatch++;
		m_isRegPlusReg = true;
		m_reg1 = Register::getRegister(p_annotation.getTarget().substr(0,3));
		m_reg2 = Register::getRegister(p_annotation.getTarget().substr(4,3));

		if (m_reg1 != Register::UNKNOWN && m_reg2 != Register::UNKNOWN)
		{
			m_isValid = true;
			cerr << "leapattern: reg+reg:" << Register::toString(m_reg1) << " " << Register::toString(m_reg2) << endl;  
		}
	}
    
	if(regexec(&m_regex_reg_times_reg, p_annotation.getTarget().c_str(), max, pmatch, 0)==0)
	{
		// pattern is of the form reg*reg, e.g.:   EDX*EAX
		// nb: is this even a valid pattern -- not used 
		countMatch++;
		m_isRegPlusReg = true;
		m_reg1 = Register::getRegister(p_annotation.getTarget().substr(0,3));
		m_reg2 = Register::getRegister(p_annotation.getTarget().substr(4,3));

		if (m_reg1 != Register::UNKNOWN && m_reg2 != Register::UNKNOWN)
		{
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
		countMatch++;
		m_isRegPlusConstant = true;
		m_reg1 = Register::getRegister(p_annotation.getTarget().substr(0,3));

		stringstream constantSS(p_annotation.getTarget().substr(4));
		if (constantSS >> m_constant)
		{
			m_isValid = true;
			cerr << "leapattern: reg+-constant: stream: " << p_annotation.getTarget().substr(4) << " constant: " << dec << m_constant << " annotation: " << p_annotation.toString() << endl;
		}

	}
	
	if(regexec(&m_regex_reg_times_constant, p_annotation.getTarget().c_str(), max, pmatch, 0)==0)
	{
		countMatch++;
		// pattern is of the form: reg*constant, e.g.: EDX*4
		// note that constant value of annotation is in decimal (not hex)
		m_isRegTimesConstant = true;
		m_reg1 = Register::getRegister(p_annotation.getTarget().substr(0,3));
		stringstream constantSS(p_annotation.getTarget().substr(4));
		if (constantSS >> m_constant)
		{
		cerr << "leapattern: reg*constant: stream: " << p_annotation.getTarget().substr(4) << " constant: " << dec << m_constant << " annotation: " << p_annotation.toString() << endl;
			m_isValid = true;
		}
	}

	if (countMatch == 0 || countMatch > 1)
	{
		cerr << "leapattern: fail sanity check -- there are more than 1 pattern that match" << endl;
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
