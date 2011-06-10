/*
ELFINote.cpp - ELF note section reader functionality.
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


ELFINoteReader::ELFINoteReader( const IELFI* pIELFI, const IELFISection* pSection ) :
    ELFIReaderImpl( pIELFI, pSection )
{
    ProcessSection();
}


ELFINoteReader::~ELFINoteReader()
{
}


Elf32_Word
ELFINoteReader::GetNotesNum() const
{
    return m_beginPtrs.size();
}


ELFIO_Err
ELFINoteReader::GetNote( Elf32_Word   index,
                         Elf32_Word&  type,
                         std::string& name,
                         void*& desc,
                         Elf32_Word& descSize ) const
{
    if ( index >= m_pSection->GetSize() ) {
        return ERR_ELFIO_INDEX_ERROR;
    }
    
    const char* pData = m_pSection->GetData() + m_beginPtrs[index];
    
    type = Convert32Word2Host( *(Elf32_Word*)( pData + 2*sizeof( Elf32_Word ) ),
                               m_pIELFI->GetEncoding() );
    Elf32_Word namesz = Convert32Word2Host( *(Elf32_Word*)( pData ),
                                            m_pIELFI->GetEncoding() );
    name.assign( pData + 3*sizeof( Elf32_Word ), namesz );
    Elf32_Word descsz = Convert32Word2Host( *(Elf32_Word*)( pData + sizeof( namesz ) ),
                                            m_pIELFI->GetEncoding() );
    if ( 0 == descsz ) {
        desc = 0;
    }
    else {
        desc = const_cast<char*> ( pData + 3*sizeof( Elf32_Word ) +
                                   ( namesz + sizeof( Elf32_Word ) - 1 ) / sizeof( Elf32_Word ) *
                                   sizeof( Elf32_Word ) );
    }
    
    return ERR_ELFIO_NO_ERROR;
}


void
ELFINoteReader::ProcessSection()
{
    const char* pData   = m_pSection->GetData();
    int         size    = m_pSection->GetSize();
    Elf32_Word  current = 0;
    
    m_beginPtrs.clear();
    
    // Is it empty?
    if ( 0 == pData || 0 == size ) {
        return;
    }

    while ( current + 3*sizeof( Elf32_Word ) <= size ) {
        m_beginPtrs.push_back( current );
        Elf32_Word namesz = Convert32Word2Host( *(Elf32_Word*)( pData + current ),
                                                m_pIELFI->GetEncoding() );
        Elf32_Word descsz = Convert32Word2Host( *(Elf32_Word*)( pData + current + sizeof( namesz ) ),
                                                m_pIELFI->GetEncoding() );
        current += 3*sizeof( Elf32_Word ) +
                   ( namesz + sizeof( Elf32_Word ) - 1 ) / sizeof( Elf32_Word ) *
                       sizeof( Elf32_Word ) + 
                   ( descsz + sizeof( Elf32_Word ) - 1 ) / sizeof( Elf32_Word ) *
                       sizeof( Elf32_Word );
    }
}
