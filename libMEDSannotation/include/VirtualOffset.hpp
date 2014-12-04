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

#ifndef _VIRTUAL_OFFSET_H_
#define _VIRTUAL_OFFSET_H_

#include <string>
#include <sstream>

typedef unsigned ApplicationAddress;

#define DEFAULT_LIBRARY_NAME "a.out"

class VirtualOffset 
{
	public:
		VirtualOffset();
		VirtualOffset(const std::string &p_offset, const std::string &p_libraryName);
		VirtualOffset(const std::string &p_offset);
		VirtualOffset(const int p_offset);

		ApplicationAddress getOffset() const;
		std::string getLibraryName() const;

		bool operator < (const VirtualOffset &p_other) const;
		bool operator == (const VirtualOffset &p_other) const;
		VirtualOffset& operator = (const VirtualOffset &p_other);

		const std::string to_string() const { std::ostringstream oss; oss<<getLibraryName() << "+0x"<<std::hex<<getOffset(); return oss.str(); }

	private:
		ApplicationAddress   m_offset;
		std::string          m_libraryName;
};

#endif
