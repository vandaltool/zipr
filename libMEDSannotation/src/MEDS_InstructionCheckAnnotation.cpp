#include <iostream>
#include <cstdio>
#include <string>

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
*/

MEDS_InstructionCheckAnnotation::MEDS_InstructionCheckAnnotation(const std::string &p_rawInputLine)
{
	m_isValid = false;
	m_rawInputLine = p_rawInputLine;
	m_isOverflow = false;
	m_isUnderflow = false;
	m_isTruncation = false;
	m_isSignedness = false;
	m_isSigned = false;
	m_isUnsigned = false;
	m_isUnknownSign = true;
	m_bitWidth = -1;
	m_truncationFromWidth = -1;
	m_truncationToWidth = -1;
	m_register = Register::UNKNOWN;

	parse();
}

// parse and set all the member variables
void MEDS_InstructionCheckAnnotation::parse()
{

	// format:
	//	field 1 - instruction address
	//  field 2 - instruction size (ignore)
	//  field 3 - INSTR
	//  field 4 - CHECK
	//  field 5 - {OVERFLOW | UNDERFLOW | SIGNEDNESS | TRUNCATION }  
	//  field 6 - {SIGNED | UNSIGNED | UNKNOWNSIGN | 16 | 32}
	//  field 7 - {<register> | <memory reference>}

	if (m_rawInputLine.find(MEDS_ANNOT_INSTR)==string::npos || m_rawInputLine.find(MEDS_ANNOT_CHECK)==string::npos) 
		return;

	// get offset
	VirtualOffset vo(m_rawInputLine);
	m_virtualOffset = vo;

	// The annotation format is very simple so we don't bother with any fancy parsing
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

	// get bit width information for overflow & underflow
	if (m_isOverflow || m_isUnderflow)
	{
		sscanf(m_rawInputLine.c_str(), "%*s %*d %*s %*s %*s %*s %d", &m_bitWidth);
	}
	else if (m_isTruncation) // get bid width from/to information for truncation
	{
		char buf[1024] = "";
		// [ADDR] [SIZE] INSTR CHECK TRUNCATION UNKNOWNSIGN 32 EAX 16 AX ZZ mov     [esp+2Ah], ax
		sscanf(m_rawInputLine.c_str(), "%*s %*d %*s %*s %*s %*s %d %s %d", &m_truncationFromWidth, buf, &m_truncationToWidth);
		m_register = Register::getRegister(string(buf));
	}

	m_isValid = true;
	
}

VirtualOffset MEDS_InstructionCheckAnnotation::getVirtualOffset() const
{
	return m_virtualOffset;
}
