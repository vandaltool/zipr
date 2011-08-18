#include <iostream>
#include <string.h>
#include "elfreader.h"

using namespace std;

ElfReader::ElfReader(char *p_elfFile)
{
    ELFIO::GetInstance()->CreateELFI( &m_reader );

    // Initialize it
    ELFIO_Err err = m_reader->Load( p_elfFile );
    if ( ERR_ELFIO_NO_ERROR != err ) {
        std::cerr << "Can't open file" << std::endl;
    }

    // List all sections of the file
    int i;
    int nSecNo = m_reader->GetSectionsNum();
    for ( i = 0; i < nSecNo; ++i ) 
    {    // For all sections
        const IELFISection* pSec = m_reader->GetSection( i );
        m_sections.push_back(pSec);
#if 0
        std::cout << "Sec. name: " << pSec->GetName() 
                  << " Sec. offset: " << pSec->GetOffset() 
                  << " Sec. size: " << pSec->GetSize() << std::endl;
#endif

    }
    std::cout << std::endl;
}

ElfReader::~ElfReader()
{
  for ( int i = 0; i < m_reader->GetSectionsNum(); ++i ) 
  {
    m_sections[i]->Release();
  }

  m_reader->Release();
}

/*
* Read <p_numBytes> from ELF file for location <p_pc>
*/
string ElfReader::read(app_iaddr_t p_pc, unsigned p_numBytes)
{
  for ( int i = 0; i < m_reader->GetSectionsNum(); ++i ) 
  {    
    const IELFISection* pSec = m_reader->GetSection( i );

/*
    cerr << "Sec. name: " << pSec->GetName() 
         << " Sec. address: " << pSec->GetAddress() 
         << " Sec. offset: " << pSec->GetOffset() 
         << " Sec. size: " << pSec->GetSize() << std::endl;
*/

    if (pSec->GetAddress() + pSec->GetSize() < 1) continue;
    if (p_pc >= pSec->GetAddress() && p_pc <= (pSec->GetAddress() + pSec->GetSize() - 1))
    {
      // found the section, now read off the data
      long offset = p_pc - pSec->GetAddress();
//      cerr << "ElfReader::read(): pc 0x" << hex << p_pc << " is in section#" << i << ": " << pSec->GetName() << " at offset: " << offset << endl;
      for (int j = 0; j < p_numBytes; ++j)
      {
        unsigned char c = pSec->GetData()[j + offset];
      }

      cerr << endl;
      return string(pSec->GetData() + offset, p_numBytes);
    }
  }

  return string();
}

/*
* Read <p_numBytes> from ELF file for location <p_pc>
* No bounds checking is done on <p_buf>
* Return false if address not in valid sections
*/
bool ElfReader::read(app_iaddr_t p_pc, unsigned p_numBytes, char* p_buf)
{
  for ( int i = 0; i < m_reader->GetSectionsNum(); ++i ) 
  {    
    const IELFISection* pSec = m_reader->GetSection( i );

    if (pSec->GetAddress() + pSec->GetSize() < 1) continue;
    if (p_pc >= pSec->GetAddress() && p_pc <= (pSec->GetAddress() + pSec->GetSize() - 1))
    {
      // found the section, now read off the data
      long offset = p_pc - pSec->GetAddress();
      memcpy(p_buf, pSec->GetData() + offset, p_numBytes);
      return true;
    }
  }
  return false;
}

/*
* Return buffer for instruction off the ELF file
*/
char* ElfReader::getInstructionBuffer(app_iaddr_t p_pc)
{
  for ( int i = 0; i < m_reader->GetSectionsNum(); ++i ) 
  {    
    const IELFISection* pSec = m_reader->GetSection( i );

    if (pSec->GetAddress() + pSec->GetSize() < 1) continue;
    if (p_pc >= pSec->GetAddress() && p_pc <= (pSec->GetAddress() + pSec->GetSize() - 1))
    {
      // found the section, now read off the data
      long offset = p_pc - pSec->GetAddress();
      return (char*) (pSec->GetData() + offset);
    }
  }
  return NULL;
}

