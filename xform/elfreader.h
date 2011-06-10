#ifndef _elfreader_H_
#define _elfreader_H_

#include <vector>
#include "config.h"
#include "ELFIO.h"

using namespace std;

class ElfReader 
{
  public:
    ElfReader(char *);
    virtual ~ElfReader();

    string read(app_iaddr_t p_pc, unsigned p_numBytes);
    bool read(app_iaddr_t p_pc, unsigned p_numBytes, char* p_buf);
    char* getInstructionBuffer(app_iaddr_t p_pc);

  private:
    IELFI*                       m_reader;
    vector < const IELFISection * >    m_sections;
};

#endif
