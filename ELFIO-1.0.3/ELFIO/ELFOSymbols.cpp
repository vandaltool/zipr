/*
ELFOSymbols.cpp - ELF relocation table section producer functionality.
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


ELFOSymbolTable::ELFOSymbolTable( IELFO* pIELFO, IELFOSection* pSection ) :
        m_nRefCnt( 1 ),
        m_pIELFO( pIELFO ),
        m_pSection( pSection )
{
    m_pIELFO->AddRef();
    m_pSection->AddRef();

    if ( 0 == m_pSection->GetSize() ) {
        Elf32_Sym entry;
        entry.st_name  = 0;
        entry.st_value = 0;
        entry.st_size  = 0;
        entry.st_info  = 0;
        entry.st_other = 0;
        entry.st_shndx = 0;
    
        ELFIO_Err err =
            m_pSection->AddData( reinterpret_cast<char*>( &entry ), sizeof( entry ) );
    }
}


ELFOSymbolTable::~ELFOSymbolTable()
{
}


int
ELFOSymbolTable::AddRef()
{
    m_pIELFO->AddRef();
    m_pSection->AddRef();
    return ++m_nRefCnt;
}


int
ELFOSymbolTable::Release()
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


Elf32_Word
ELFOSymbolTable::AddEntry( Elf32_Word name, Elf32_Addr value, Elf32_Word size,
                           unsigned char info, unsigned char other,
                           Elf32_Half shndx )
{
    Elf32_Sym entry;
    entry.st_name  = Convert32Word2Host( name, m_pIELFO->GetEncoding() );
    entry.st_value = Convert32Addr2Host( value, m_pIELFO->GetEncoding() );
    entry.st_size  = Convert32Word2Host( size, m_pIELFO->GetEncoding() );;
    entry.st_info  = info;
    entry.st_other = other;
    entry.st_shndx = Convert32Half2Host( shndx, m_pIELFO->GetEncoding() );
    
    ELFIO_Err err =
        m_pSection->AddData( reinterpret_cast<char*>( &entry ), sizeof( entry ) );

    Elf32_Word nRet = m_pSection->GetSize() / sizeof(Elf32_Sym) - 1;
    
    return nRet;
}


Elf32_Word
ELFOSymbolTable::AddEntry( Elf32_Word name, Elf32_Addr value, Elf32_Word size,
                           unsigned char bind, unsigned char type, unsigned char other,
                           Elf32_Half shndx )
{
    return AddEntry( name, value, size, ELF32_ST_INFO( bind, type ), other, shndx );
}


Elf32_Word
ELFOSymbolTable::AddEntry( IELFOStringWriter* pStrWriter, const char* str,
                                 Elf32_Addr value, Elf32_Word size,
                                 unsigned char info, unsigned char other,
                                 Elf32_Half shndx )
{
    Elf32_Word index = pStrWriter->AddString( str );
    return AddEntry( index, value, size, info, other, shndx );
}


Elf32_Word
ELFOSymbolTable::AddEntry( IELFOStringWriter* pStrWriter, const char* str,
                                 Elf32_Addr value, Elf32_Word size,
                                 unsigned char bind, unsigned char type, unsigned char other,
                                 Elf32_Half shndx )
{
    return AddEntry( pStrWriter,str, value, size, ELF32_ST_INFO( bind, type ), other, shndx );
}
