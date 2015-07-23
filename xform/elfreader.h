#ifndef _elfreader_H_
#define _elfreader_H_

#include <vector>
// #include "elfio/elfio.hpp"
#include "exeio.h"
#include "targ-config.h"
#include <assert.h>


// doing this is very bad.
// using namespace std;
// using namespace ELFIO;

class ElfReader 
{
  public:
    ElfReader(char *);
    virtual ~ElfReader();

    std::string read(app_iaddr_t p_pc, unsigned p_numBytes);
    bool read(app_iaddr_t p_pc, unsigned p_numBytes, char* p_buf);
    char* getInstructionBuffer(app_iaddr_t p_pc);

    bool isElf32() { assert(m_reader); return m_reader->get_class()==EXEIO::ELF32; }
    bool isElf64() { assert(m_reader); return m_reader->get_class()==EXEIO::ELF64; }
    bool isPe32() { assert(m_reader); return m_reader->get_class()==EXEIO::PE32; }
    bool isPe64() { assert(m_reader); return m_reader->get_class()==EXEIO::PE64; }

  private:
//    ELFIO::elfio*                       m_reader;
    EXEIO::exeio*                       m_reader;
    
//    std::vector < const ELFIO::section* >    m_sections;

};

#endif
