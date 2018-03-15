/*
 * Copyright (c) 2014, 2015 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

#include <iostream>
#include <string.h>
#include "targ-config.h"

#include <stdio.h>

/*
#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"

*/
#include "elfreader.h"

using namespace std;
//using namespace ELFIO;
using namespace EXEIO;

ElfReader::ElfReader(char *p_elfFile)
{
//    m_reader=new elfio;
	m_reader=new EXEIO::exeio(p_elfFile);
	assert(m_reader);

	EXEIO::dump::header(cout, *m_reader);
	EXEIO::dump::section_headers(cout, *m_reader);

    // Initialize it
/*
    bool ok = m_reader->load( p_elfFile );
    if ( ! ok ) {
        std::cerr << "Can't open file:" << p_elfFile << std::endl;
	assert(0);
	exit(-1);
    }


    if(m_reader->get_class() == ELFCLASS32)
	std::cout << "Input file is ELF32" << std::endl;
    else
	std::cout << "Input file is ELF64" << std::endl;


    // List all sections of the file
    int i;
    ELFIO::Elf_Half nSecNo = m_reader->sections.size();
    for ( i = 0; i < nSecNo; ++i ) 
    {    // For all sections
	section* psec = m_reader->sections[i];
	m_sections.push_back(psec);
        std::cout << "  [" << i << "] "
                  << psec->get_name()
                  << "\t"
                  << psec->get_size()
                  << std::endl;

    }
    std::cout << std::endl;
*/
}

ElfReader::~ElfReader()
{
  delete m_reader;
}

/*
* Read <p_numBytes> from ELF file for location <p_pc>
*/
string ElfReader::read(app_iaddr_t p_pc, unsigned p_numBytes)
{
  for ( int i = 0; i < m_reader->sections.size(); ++i ) 
  {    
    section* pSec = m_reader->sections[i];
    if (pSec->get_address() + pSec->get_size() < 1) continue;
    if (p_pc >= pSec->get_address() && p_pc <= (pSec->get_address() + pSec->get_size() - 1))
    {
      // found the section, now read off the data
      long offset = p_pc - pSec->get_address();

      // caution!  This may result in an overflow of the section and fault if called with p_numBytes too big.
      return string(pSec->get_data() + offset, p_numBytes);
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
  for ( int i = 0; i < m_reader->sections.size(); ++i ) 
  {    
    section* pSec = m_reader->sections[ i ];

    if (pSec->get_address() + pSec->get_size() < 1) continue;
    if (p_pc >= pSec->get_address() && p_pc <= (pSec->get_address() + pSec->get_size() - 1))
    {
      // found the section, now read off the data
      long offset = p_pc - pSec->get_address();
      memcpy(p_buf, pSec->get_data() + offset, p_numBytes);
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
  for ( int i = 0; i < m_reader->sections.size(); ++i ) 
  {    
    const section* pSec = m_reader->sections[ i ];

    if (pSec->get_address() + pSec->get_size() < 1) continue;
    if (p_pc >= pSec->get_address() && p_pc <= (pSec->get_address() + pSec->get_size() - 1))
    {
      // found the section, now read off the data
      long offset = p_pc - pSec->get_address();
      return (char*) (pSec->get_data() + offset);
    }
  }
  return NULL;
}

