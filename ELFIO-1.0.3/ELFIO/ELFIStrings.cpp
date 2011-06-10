/*
ELFIStrings.cpp - ELF string section reader functionality.
Copyright (C) 2001 Serge Lamikhov-Center <to_serge@users.sourceforge.net>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "ELFIImpl.h"
#include "ELFIOUtils.h"


ELFIStringReader::ELFIStringReader( const IELFI* pIELFI, const IELFISection* pSection ) :
    ELFIReaderImpl( pIELFI, pSection )
{
}


ELFIStringReader::~ELFIStringReader()
{
}


const char*
ELFIStringReader::GetString( Elf32_Word index ) const
{
    if ( index < m_pSection->GetSize() ) {
        const char* pData = m_pSection->GetData();
        if ( 0 != pData ) {
            return pData + index;
        }
    }
    
    return 0;
}
