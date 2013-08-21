#ifndef _elfreader_H_
#define _elfreader_H_

#include <vector>
#include "elfio/elfio.hpp"
#include "targ-config.h"
#include <assert.h>

using namespace std;
using namespace ELFIO;

class ElfReader 
{
  public:
    ElfReader(char *);
    virtual ~ElfReader();

    string read(app_iaddr_t p_pc, unsigned p_numBytes);
    bool read(app_iaddr_t p_pc, unsigned p_numBytes, char* p_buf);
    char* getInstructionBuffer(app_iaddr_t p_pc);

    bool isElf64() { assert(m_reader); return m_reader->get_class()==ELFCLASS64; }





  private:
    elfio*                       m_reader;
    vector < const section* >    m_sections;

};

#endif
