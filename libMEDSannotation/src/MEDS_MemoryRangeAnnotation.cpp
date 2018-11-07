/*
 * Copyright (c) 2018 - University of Virginia
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
 * Author: University of Virginia
 * e-mail: jwd@virginia.edu
 *
 */

#include <stdlib.h>

#include <iostream>
#include <cstdio>
#include <string>
#include <string.h>
#include <cinttypes>

#include "MEDS_MemoryRangeAnnotation.hpp"

using namespace std;
using namespace MEDS_Annotation;

/*
	Example format -- subject to change:

	417748     12 INSTR STATICMEMWRITE MIN 3c60320  LIMIT 4e53730  ZZ
	4992ea      4 INSTR STACKMEMRANGE MIN RSP-568 LIMIT RSP-48 INSTRSPDELTA -592 ZZ

	See explanations in the header file MEDS_MemoryRangeAnnotation.hpp.

*/

MEDS_MemoryRangeAnnotation::MEDS_MemoryRangeAnnotation() : MEDS_AnnotationBase()
{
	this->setInvalid();	
	this->setStackRange(false);
	this->setStaticGlobalRange(false);
}

MEDS_MemoryRangeAnnotation::MEDS_MemoryRangeAnnotation(const string &p_rawLine) : MEDS_AnnotationBase()
{
	this->setInvalid();
	this->setStackRange(false);
	this->setStaticGlobalRange(false);
	this->m_rawInputLine = p_rawLine;
	this->parse();
}

void MEDS_MemoryRangeAnnotation::parse()
{
	if (this->m_rawInputLine.find(" INSTR ") == string::npos)
		return;

	if (this->m_rawInputLine.find(MEDS_ANNOT_STATICMEMWRITE) != string::npos)
	{
		this->setStaticGlobalRange(true);
	}

	if (this->m_rawInputLine.find(MEDS_ANNOT_STACKMEMRANGE) != string::npos)
	{
		this->setStackRange(true);
	}

	if (!this->isStackRange() && !this->isStaticGlobalRange())
	{
		/* invalid annotation */
		this->setInvalid();	
		return;
	}

	// get offset
	VirtualOffset vo(m_rawInputLine);
	this->setVirtualOffset(vo); // in base class

	uint64_t MinVal, LimitVal;
	int instrSize;

	// 417748     12 INSTR STATICMEMWRITE MIN 3c60320  LIMIT 4e53730  ZZ
	// 4992ea      4 INSTR STACKMEMRANGE MIN RSP - 568 LIMIT RSP - 48 INSTRSPDELTA - 592 ZZ
	if (this->isStaticGlobalRange()) {
		int ItemsFilled = sscanf(m_rawInputLine.c_str(), "%*x %d %*s %*s MIN %" SCNx64 " LIMIT %" SCNx64, &instrSize, &MinVal, &LimitVal);
		if (3 != ItemsFilled) {
			this->setInvalid();
			cerr << "Error on sscanf of annotation: ItemsFilled = " << ItemsFilled << " line: " << m_rawInputLine << endl;
			return;
		}
		else {
			cerr << "Parsed STATICMEMWRITE annotation: MIN = " << hex << MinVal << " LIMIT = " << LimitVal << endl;
		}
	}
	else {
#if 0
		int ItemsFilled = sscanf(m_rawInputLine.c_str(), "%*x %d %*s %*s MIN %" SCNx64 " LIMIT %" SCNx64, &instrSize, &MinVal, &LimitVal);
		if (3 != ItemsFilled) {
			this->setInvalid();
			cerr << "Error on sscanf of annotation: ItemsFilled = " << ItemsFilled << " line: " << m_rawInputLine << endl;
			return;
		}
#else
		this->setInvalid();
		cerr << "Not yet parsing STACKMEMRANGE annotations " << endl;
		return;
#endif
	}

	this->setInstructionSize(instrSize); // in base class
	this->setRangeMin(MinVal);
	this->setRangeLimit(LimitVal);

	cout << "virtual offset: " << hex << this->getVirtualOffset().getOffset() << dec << endl;
	cout << "size: " << this->getInstructionSize() << endl;
	cout << "min: " << this->getRangeMin() << endl;
	cout << "limit: " << this->getRangeLimit() << endl;

	if (LimitVal <= MinVal)
	{
		setInvalid();
		cerr << "invalid range limit" << endl;
		return;
	}

	cout << "valid annotation" << endl;
	this->setValid();
} // end of MEDS_MemoryRangeAnnotation::parse()

