/*
ELFONote.cpp - ELF note section producer.
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


#include "ELFOImpl.h"
#include "ELFIOUtils.h"


ELFONotesWriter::ELFONotesWriter( IELFO* pIELFO, IELFOSection* pSection ) :
        m_nRefCnt( 1 ),
        m_pIELFO( pIELFO ),
        m_pSection( pSection )
{
    m_pIELFO->AddRef();
    m_pSection->AddRef();
}


ELFONotesWriter::~ELFONotesWriter()
{
}


int
ELFONotesWriter::AddRef()
{
    m_pIELFO->AddRef();
    m_pSection->AddRef();
    return ++m_nRefCnt;
}


int
ELFONotesWriter::Release()
{
    int nRet             = --m_nRefCnt;
    IELFO*        pIELFO = m_pIELFO;
    IELFOSection* pSec   = m_pSection;

    if ( 0 == m_nRefCnt ) {
        delete this;
    }
    pSec->Release();
    pIELFO->Release();

    return nRet;
}


ELFIO_Err
ELFONotesWriter::AddNote( Elf32_Word type, const std::string& name,
                          const void* desc, Elf32_Word descSize )
{
    Elf32_Word nameLen = name.size() + 1;
    Elf32_Word nameLenConv = Convert32Word2Host( nameLen, m_pIELFO->GetEncoding() );
    std::string buffer( reinterpret_cast<char*>( &nameLenConv ), sizeof( nameLenConv ) );
    Elf32_Word descSizeConv = Convert32Word2Host( descSize, m_pIELFO->GetEncoding() );
    buffer.append( reinterpret_cast<char*>( &descSizeConv ), sizeof( descSizeConv ) );
    type = Convert32Word2Host( type, m_pIELFO->GetEncoding() );
    buffer.append( reinterpret_cast<char*>( &type ), sizeof( type ) );
    buffer.append( name );
    const char pad[] = { '\0', '\0', '\0', '\0' };
    if ( nameLen % sizeof( Elf32_Word ) != 0 ) {
        buffer.append( pad, sizeof( Elf32_Word ) -
                            nameLen % sizeof( Elf32_Word ) );
    }
    if ( desc != 0 && descSize != 0 ) {
        buffer.append( reinterpret_cast<const char*>( desc ), descSize );
        if ( descSize % sizeof( Elf32_Word ) != 0 ) {
            buffer.append( pad, sizeof( Elf32_Word ) -
                                descSize % sizeof( Elf32_Word ) );
        }
    }
    
    return m_pSection->AddData( buffer );
}
