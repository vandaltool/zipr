/*
ELFORelocation.cpp - ELF relocation table section producer functionality.
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


ELFORelocationTable::ELFORelocationTable( IELFO* pIELFO, IELFOSection* pSection ) :
        m_nRefCnt( 1 ),
        m_pIELFO( pIELFO ),
        m_pSection( pSection )
{
    m_pIELFO->AddRef();
    m_pSection->AddRef();
}


ELFORelocationTable::~ELFORelocationTable()
{
}


int
ELFORelocationTable::AddRef()
{
    m_pIELFO->AddRef();
    m_pSection->AddRef();
    return ++m_nRefCnt;
}


int
ELFORelocationTable::Release()
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
ELFORelocationTable::AddEntry( Elf32_Addr offset, Elf32_Word info )
{
    Elf32_Rel entry;
    entry.r_offset = Convert32Addr2Host( offset, m_pIELFO->GetEncoding() );
    entry.r_info   = Convert32Word2Host( info, m_pIELFO->GetEncoding() );
    
    ELFIO_Err err;
    err = m_pSection->AddData( reinterpret_cast<char*>( &entry ), sizeof( entry ) );
    
    return err;
}


ELFIO_Err
ELFORelocationTable::AddEntry( Elf32_Addr offset, Elf32_Word symbol,
                               unsigned char type )
{
    Elf32_Word info = ELF32_R_INFO( symbol, type );
    return AddEntry( offset, info );
}


ELFIO_Err
ELFORelocationTable::AddEntry( Elf32_Addr offset, Elf32_Word info,
               Elf32_Sword addend )
{
    Elf32_Rela entry;
    entry.r_offset = Convert32Addr2Host( offset, m_pIELFO->GetEncoding() );
    entry.r_info   = Convert32Word2Host( info, m_pIELFO->GetEncoding() );
    entry.r_addend = Convert32Sword2Host( addend, m_pIELFO->GetEncoding() );
    
    ELFIO_Err err;
    err = m_pSection->AddData( reinterpret_cast<char*>( &entry ), sizeof( entry ) );
    
    return err;
}


ELFIO_Err
ELFORelocationTable::AddEntry( Elf32_Addr offset, Elf32_Word symbol,
                           unsigned char type, Elf32_Sword addend )
{
    Elf32_Word info = ELF32_R_INFO( symbol, type );
    return AddEntry( offset, info, addend );
}


ELFIO_Err
ELFORelocationTable::AddEntry( IELFOStringWriter* pStrWriter, const char* str,
                               IELFOSymbolTable* pSymWriter,
                               Elf32_Addr value, Elf32_Word size,
                               unsigned char symInfo, unsigned char other,
                               Elf32_Half shndx,
                               Elf32_Addr offset, unsigned char type )
{
    // Add string
    Elf32_Word nStrIndex = pStrWriter->AddString( str );

    // Add symbol
    Elf32_Word nSymIndex =
        pSymWriter->AddEntry( nStrIndex, value, size, symInfo, other, shndx );

    // Add relocation entry
    return AddEntry( offset, ELF32_R_INFO( nSymIndex, type ) );
}


ELFIO_Err
ELFORelocationTable::AddEntry( IELFOStringWriter* pStrWriter, const char* str,
                               IELFOSymbolTable* pSymWriter,
                               Elf32_Addr value, Elf32_Word size,
                               unsigned char symInfo, unsigned char other,
                               Elf32_Half shndx,
                               Elf32_Addr offset, unsigned char type,
                               Elf32_Sword addend )
{
    // Add string
    Elf32_Word nStrIndex = pStrWriter->AddString( str );

    // Add symbol
    Elf32_Word nSymIndex =
        pSymWriter->AddEntry( nStrIndex, value, size, symInfo, other, shndx );

    // Add relocation entry
    return AddEntry( offset, ELF32_R_INFO( nSymIndex, type ), addend );
}
