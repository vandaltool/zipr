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
#include <stdio.h>
#include <irdb-core>
#include <libIRDB-core.hpp>
#include "elfreader.h"

using namespace std;
using namespace IRDB_SDK;
using namespace EXEIO;

ElfReader::ElfReader(char *p_elfFile)
{
	m_reader=new EXEIO::exeio(p_elfFile);
	assert(m_reader);

	EXEIO::dump::header(cout, *m_reader);
	EXEIO::dump::section_headers(cout, *m_reader);

}

ElfReader::~ElfReader()
{
  delete m_reader;
}

/*
* Read <p_numBytes> from ELF file for location <p_pc>
*/
string ElfReader::read(VirtualOffset_t p_pc, unsigned p_numBytes) const
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
bool ElfReader::read(VirtualOffset_t p_pc, unsigned p_numBytes, char* p_buf) const
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
const char* ElfReader::getInstructionBuffer(VirtualOffset_t p_pc) const
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

void ElfReader::SetArchitecture()
{
	const auto width =
		(isElf32() || isPe32()) ? 32 :
		(isElf64() || isPe64()) ? 64 :
		throw std::invalid_argument("Unknown architecture.");
	const auto mt = m_reader->getMachineType() == EXEIO::mtI386    ? IRDB_SDK::admtI386    :
			m_reader->getMachineType() == EXEIO::mtX86_64  ? IRDB_SDK::admtX86_64  :
			m_reader->getMachineType() == EXEIO::mtAarch64 ? IRDB_SDK::admtAarch64 :
			m_reader->getMachineType() == EXEIO::mtArm32   ? IRDB_SDK::admtArm32   :
			m_reader->getMachineType() == EXEIO::mtMips32  ? IRDB_SDK::admtMips32  :
			m_reader->getMachineType() == EXEIO::mtMips64  ? IRDB_SDK::admtMips64  :
			throw std::invalid_argument("Unknown architecture.");

	libIRDB::FileIR_t::setArchitecture(width,mt);
}

