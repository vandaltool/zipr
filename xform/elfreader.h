#ifndef _elfreader_H_
#define _elfreader_H_

#include <vector>
#include "exeio.h"
#include "targ-config.h"
#include <assert.h>
#include <exception>
#include <libIRDB-core.hpp>


// doing this is very bad.
// using namespace std;
// using namespace ELFIO;

class ElfReader 
{
	public:

	ElfReader(char *);
	virtual ~ElfReader();

	std::string read(app_iaddr_t p_pc, unsigned p_numBytes) const ;
	bool read(app_iaddr_t p_pc, unsigned p_numBytes, char* p_buf) const ;
	const char* getInstructionBuffer(app_iaddr_t p_pc) const ;

	bool isElf32() const { assert(m_reader); return m_reader->get_class()==EXEIO::ELF32; }
	bool isElf64() const { assert(m_reader); return m_reader->get_class()==EXEIO::ELF64; }
	bool isPe32() const { assert(m_reader); return m_reader->get_class()==EXEIO::PE32; }
	bool isPe64() const { assert(m_reader); return m_reader->get_class()==EXEIO::PE64; }
	void SetArchitecture() 
	{ 
		const auto width = 
			(isElf32() || isPe32()) ? 32 : 
			(isElf64() || isPe64()) ? 64 :
			throw std::invalid_argument("Unknown architecture."); 
		const auto mt = m_reader->getMachineType() == EXEIO::mtI386 ? libIRDB::admtI386 :
				m_reader->getMachineType() == EXEIO::mtX86_64 ? libIRDB::admtX86_64 :
				m_reader->getMachineType() == EXEIO::mtAarch64 ? libIRDB::admtAarch64 : 
				throw std::invalid_argument("Unknown architecture."); 

		libIRDB::FileIR_t::SetArchitecture(width,mt); 
	}

  	private:
	EXEIO::exeio*                       m_reader;

};

#endif
