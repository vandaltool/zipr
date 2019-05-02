#ifndef _elfreader_H_
#define _elfreader_H_

#include <vector>
#include "exeio.h"
#include <irdb-core>
#include <assert.h>
#include <exception>
#include <irdb-core>


// doing this is very bad.
// using namespace std;
// using namespace ELFIO;

class ElfReader 
{
	public:

	ElfReader(char *);
	virtual ~ElfReader();

	std::string read(IRDB_SDK::VirtualOffset_t p_pc, unsigned p_numBytes) const ;
	bool read(IRDB_SDK::VirtualOffset_t p_pc, unsigned p_numBytes, char* p_buf) const ;
	const char* getInstructionBuffer(IRDB_SDK::VirtualOffset_t p_pc) const ;

	bool isElf32() const { assert(m_reader); return m_reader->get_class()==EXEIO::ELF32; }
	bool isElf64() const { assert(m_reader); return m_reader->get_class()==EXEIO::ELF64; }
	bool isPe32()  const { assert(m_reader); return m_reader->get_class()==EXEIO::PE32 ; }
	bool isPe64()  const { assert(m_reader); return m_reader->get_class()==EXEIO::PE64 ; }
	void SetArchitecture() ;

  	private:
	EXEIO::exeio*                       m_reader;

};

#endif
