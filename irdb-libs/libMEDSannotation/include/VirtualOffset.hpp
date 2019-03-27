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

namespace MEDS_Annotation
{

	using namespace std;
	using ApplicationAddress = uint64_t;

#define DEFAULT_LIBRARY_NAME "a.out"

	class VirtualOffset 
	{
		public:
			VirtualOffset();
			VirtualOffset(const string &p_offset, const string &p_libraryName);
			VirtualOffset(const string &p_offset);
			VirtualOffset(const ApplicationAddress p_offset);

			ApplicationAddress getOffset() const;
			string getLibraryName() const;

			bool operator < (const VirtualOffset &p_other) const;
			bool operator == (const VirtualOffset &p_other) const;
			VirtualOffset& operator = (const VirtualOffset &p_other);

			const string to_string() const { ostringstream oss; oss<<getLibraryName() << "+0x"<<hex<<getOffset(); return oss.str(); }
			const string toString() const { return to_string(); }

		private:
			ApplicationAddress   m_offset;
			string          m_libraryName;
	};

}

#endif
