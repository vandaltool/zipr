/*
ELFISymbols.cpp - ELF symbol table reader functionality.
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
#include <cstring>
#include <stdio.h>

ELFISymbolTable::ELFISymbolTable( const IELFI* pIELFI, const IELFISection* pSection ) :
    ELFIReaderImpl( pIELFI, pSection )
{
    const IELFISection* pStrSection = pIELFI->GetSection( GetStringTableIndex() );
    m_pIELFI->CreateSectionReader( IELFI::ELFI_STRING,
                                   pStrSection,
                                   (void**)&m_pStrReader );
    pStrSection->Release();

    // Find hash section
    m_nHashSection = 0;
    m_pHashSection = 0;
    Elf32_Half nSecNo = m_pIELFI->GetSectionsNum();
    for ( Elf32_Half i = 0; i < nSecNo && 0 == m_nHashSection; ++i ) {
        const IELFISection* pSec = m_pIELFI->GetSection( i );
        if ( pSec->GetLink() == m_pSection->GetIndex() ) {
            m_nHashSection = i;
            m_pHashSection = pSec;
            m_pHashSection->AddRef();
        }
        pSec->Release();
    }
}


ELFISymbolTable::~ELFISymbolTable()
{
}


int
ELFISymbolTable::AddRef() const
{
    m_pStrReader->AddRef();
    if ( 0 != m_pHashSection ) {
        m_pHashSection->AddRef();
    }

    return ELFIReaderImpl::AddRef();
}


int
ELFISymbolTable::Release() const
{
    m_pStrReader->Release();
    if ( 0 != m_pHashSection ) {
        m_pHashSection->Release();
    }

    return ELFIReaderImpl::Release();
}


Elf32_Half
ELFISymbolTable::GetStringTableIndex() const
{
    return (Elf32_Half)m_pSection->GetLink();
}


Elf32_Half
ELFISymbolTable::GetHashTableIndex() const
{
    return m_nHashSection;
}


Elf32_Word
ELFISymbolTable::GetSymbolNum() const
{
    Elf32_Word nRet = 0;
    if ( 0 != m_pSection->GetEntrySize() ) {
        nRet = m_pSection->GetSize() / m_pSection->GetEntrySize();
    }
    
    return nRet;
}


ELFIO_Err
ELFISymbolTable::GetSymbol( Elf32_Word index,
                            std::string& name, Elf32_Addr& value,
                            Elf32_Word& size, 
                            unsigned char& bind, unsigned char& type,
                            Elf32_Half& section ) const
{
    ELFIO_Err nRet = ERR_ELFIO_SYMBOL_ERROR;
    
    if ( index < GetSymbolNum() ) {
        const Elf32_Sym* pSym = reinterpret_cast<const Elf32_Sym*>(
                m_pSection->GetData() + index * m_pSection->GetEntrySize() );

        const char* pStr = m_pStrReader->GetString(
                  Convert32Word2Host( pSym->st_name,  m_pIELFI->GetEncoding() ) );
        if ( 0 != pStr ) {
            name = pStr;
        }
        value   = Convert32Addr2Host( pSym->st_value, m_pIELFI->GetEncoding() );
        size    = Convert32Word2Host( pSym->st_size,  m_pIELFI->GetEncoding() );
        bind    = ( pSym->st_info ) >> 4;
        type    = ( pSym->st_info ) & 0xF;
        section = Convert32Half2Host( pSym->st_shndx, m_pIELFI->GetEncoding() );

        nRet = ERR_ELFIO_NO_ERROR;
    }    
    
    return nRet;
}


ELFIO_Err
ELFISymbolTable::GetSymbol( const std::string& name,
                            Elf32_Addr& value,
                            Elf32_Word& size, 
                            unsigned char& bind, unsigned char& type,
                            Elf32_Half& section ) const
{
    ELFIO_Err nRet = ERR_ELFIO_SYMBOL_ERROR;
    
    if ( 0 != GetHashTableIndex() ) {
        Elf32_Word nbucket = *(Elf32_Word*)m_pHashSection->GetData();
        Elf32_Word val = ElfHashFunc( (const unsigned char*)name.c_str() );

        Elf32_Word y   = *(Elf32_Word*)( m_pHashSection->GetData() +
                                       ( 2 + val % nbucket ) * sizeof( Elf32_Word ) );
        std::string   str;
        GetSymbol( y, str, value, size, bind, type, section );
        while ( str != name && STN_UNDEF != y ) {
            y = *(Elf32_Word*)( m_pHashSection->GetData() +
                                    ( 2 + nbucket + y ) * sizeof( Elf32_Word ) );
            GetSymbol( y, str, value, size, bind, type, section );
        }
        if (  str == name ) {
            nRet = ERR_ELFIO_NO_ERROR;
        }
    }
    
    return nRet;
}
