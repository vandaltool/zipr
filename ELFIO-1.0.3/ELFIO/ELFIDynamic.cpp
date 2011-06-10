/*
ELFIDynamic.cpp - ELF dynamic section reader functionality.
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


ELFIDynamicReader::ELFIDynamicReader( const IELFI* pIELFI, const IELFISection* pSection ) :
    ELFIReaderImpl( pIELFI, pSection )
{
}


ELFIDynamicReader::~ELFIDynamicReader()
{
}


Elf32_Word
ELFIDynamicReader::GetEntriesNum() const
{
    Elf32_Word nRet = 0;
    if ( 0 != m_pSection->GetEntrySize() ) {
        nRet = m_pSection->GetSize() / m_pSection->GetEntrySize();
    }
    
    return nRet;
}


ELFIO_Err
ELFIDynamicReader::GetEntry( Elf32_Word   index,
                             Elf32_Sword& tag,
                             Elf32_Word&  value ) const
{
    if ( index >= GetEntriesNum() ) {    // Is index valid
        return ERR_ELFIO_INDEX_ERROR;
    }
    
    const Elf32_Dyn* pEntry = reinterpret_cast<const Elf32_Dyn*>(
            m_pSection->GetData() + index * m_pSection->GetEntrySize() );
    tag   = Convert32Sword2Host( pEntry->d_tag, m_pIELFI->GetEncoding() );
    value = Convert32Word2Host( pEntry->d_un.d_val, m_pIELFI->GetEncoding() );
    
    return ERR_ELFIO_NO_ERROR;
}
