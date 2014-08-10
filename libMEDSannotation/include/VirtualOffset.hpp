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
