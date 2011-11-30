#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sstream>

#include "leapattern.h"

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

    if(regcomp(&m_regex_reg_plus_reg ,"[eax|EAX|ebx|EBX|ecx|ECX|edx|EDX|esi|ESI|edi|EDI|ebp|EDI]\\+[eax|EAX|ebx|EBX|ecx|ECX|edx|EDX|esi|ESI|edi|EDI|ebp|EDI]", REG_EXTENDED | REG_ICASE) !=0)
	    cerr << "Error: regular expression for reg32+reg32 failed" << endl;

    if(regcomp(&m_regex_reg_times_reg ,"[eax|EAX|ebx|EBX|ecx|ECX|edx|EDX|esi|ESI|edi|EDI|ebp|EDI]\\*[eax|EAX|ebx|EBX|ecx|ECX|edx|EDX|esi|ESI|edi|EDI|ebp|EDI]", REG_EXTENDED | REG_ICASE) !=0)
	    cerr << "Error: regular expression for reg32*reg32 failed" << endl;

    if(regcomp(&m_regex_reg_plus_constant ,"[eax|EAX|ebx|EBX|ecx|ECX|edx|EDX|esi|ESI|edi|EDI|ebp|EDI]\\+[0-9+]", REG_EXTENDED | REG_ICASE) !=0)
	    cerr << "Error: regular expression for reg32+constant failed" << endl;

    if(regcomp(&m_regex_reg_plus_negconstant ,"[eax|EAX|ebx|EBX|ecx|ECX|edx|EDX|esi|ESI|edi|EDI|ebp|EDI]\\+\\-[0-9+]", REG_EXTENDED | REG_ICASE) !=0)
	    cerr << "Error: regular expression for reg32+constant failed" << endl;

    if(regcomp(&m_regex_reg_times_constant ,"[eax|EAX|ebx|EBX|ecx|ECX|edx|EDX|esi|ESI|edi|EDI|ebp|EDI]\\*[0-9+]", REG_EXTENDED | REG_ICASE) !=0)
	    cerr << "Error: regular expression for reg32+constant failed" << endl;

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
			m_isValid = true;
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
			m_isValid = true;
	}
	
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
		}

		cerr << "leapattern: reg+-constant: constant: " << m_constant << endl;
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
