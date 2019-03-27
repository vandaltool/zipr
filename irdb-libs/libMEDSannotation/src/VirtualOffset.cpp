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

#include <cstdio>
#include <iostream>
#include <inttypes.h>
#include "VirtualOffset.hpp"

using namespace std;
using namespace MEDS_Annotation;

VirtualOffset::VirtualOffset()
{
	m_offset = 0;
	m_libraryName = string(DEFAULT_LIBRARY_NAME);
}

VirtualOffset::VirtualOffset(const string &p_offset, const string &p_libraryName)
{
	sscanf(p_offset.c_str(), "%" SCNx64, &m_offset);
	m_libraryName = p_libraryName;
}

VirtualOffset::VirtualOffset(const string &p_offset)
{
	sscanf(p_offset.c_str(), "%" SCNx64, &m_offset);
	m_libraryName = string(DEFAULT_LIBRARY_NAME);
}

VirtualOffset::VirtualOffset(const ApplicationAddress p_offset)
{
	m_offset = p_offset;
	m_libraryName = string(DEFAULT_LIBRARY_NAME);
}

ApplicationAddress VirtualOffset::getOffset() const
{
	return m_offset;
}

string VirtualOffset::getLibraryName() const
{
	return m_libraryName;
}

bool VirtualOffset::operator < (const VirtualOffset &p_other) const
{
	if (&p_other == this) return false;
	return m_offset < p_other.getOffset();
}

bool VirtualOffset::operator == (const VirtualOffset &p_other) const
{
	if (&p_other == this) 
		return true;
	else
	{
		return (m_offset == p_other.getOffset() && m_libraryName == p_other.getLibraryName());
	}
}

VirtualOffset& VirtualOffset::operator = (const VirtualOffset &p_other) 
{
	if (&p_other == this)
	{
		return *this;
	}
	else
	{
		m_offset = p_other.getOffset();
		m_libraryName = p_other.getLibraryName();
		return *this;
	}
}
