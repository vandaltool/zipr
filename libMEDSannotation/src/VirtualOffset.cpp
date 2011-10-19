#include <cstdio>

#include "VirtualOffset.hpp"


VirtualOffset::VirtualOffset()
{
	m_offset = 0;
}

VirtualOffset::VirtualOffset(const std::string &p_offset, const std::string &p_libraryName)
{
	sscanf(p_offset.c_str(), "%x", &m_offset);
	m_libraryName = p_libraryName;
}


VirtualOffset::VirtualOffset(const std::string &p_offset)
{
	sscanf(p_offset.c_str(), "%x", &m_offset);
	m_libraryName = std::string(DEFAULT_LIBRARY_NAME);
}


ApplicationAddress VirtualOffset::getOffset() const
{
	return m_offset;
}

std::string VirtualOffset::getLibraryName() const
{
	return m_libraryName;
}

// /usr/include/c++/4.4/bits/stl_function.h:230: error: passing ‘const VirtualOffset’ as ‘this’ argument of ‘bool VirtualOffset::operator<(VirtualOffset)’ discards qualifiers

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
		return (m_offset == p_other.getOffset() && m_libraryName == p_other.getLibraryName());
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
