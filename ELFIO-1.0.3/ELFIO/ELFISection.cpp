/*
ELFISection.cpp - ELF section reader implementation.
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


IELFISection::~IELFISection()
{
}


ELFISection::ELFISection( IELFI* pIELFI, std::istream* pStream, int nFileOffset,
                          Elf32_Shdr* pHeader, Elf32_Half index ) :
    m_pIELFI( pIELFI ), m_pStream( pStream ), m_nFileOffset( nFileOffset), m_index( index )
{
    std::fill_n( reinterpret_cast<char*>( &m_sh ), sizeof( m_sh ), '\0' );
    m_sh      = *pHeader;
    m_data    = 0;
}


ELFISection::~ELFISection()
{
    delete [] m_data;
}


int
ELFISection::AddRef() const
{
    return m_pIELFI->AddRef();
}


int
ELFISection::Release() const
{
    return m_pIELFI->Release();
}


Elf32_Half
ELFISection::GetIndex() const
{
    return m_index;
}


std::string
ELFISection::GetName() const
{
    std::string strRet   = "";
    const char* pName    = 0;
    Elf32_Half  shstrndx = m_pIELFI->GetSecStrNdx();

    if ( SHN_UNDEF != shstrndx ) {
        const IELFISection* pStrSec = m_pIELFI->GetSection( shstrndx );
        pName = pStrSec->GetData();
        if ( 0 != pName ) {
            strRet = pName + Convert32Word2Host( m_sh.sh_name, m_pIELFI->GetEncoding() );
        }
        pStrSec->Release();
    }

    return strRet;
}


Elf32_Word
ELFISection::GetType() const
{
    return Convert32Word2Host( m_sh.sh_type, m_pIELFI->GetEncoding() );
}


Elf32_Word
ELFISection::GetFlags() const
{
    return Convert32Word2Host( m_sh.sh_flags, m_pIELFI->GetEncoding() );
}


Elf32_Addr
ELFISection::GetAddress() const
{
    return Convert32Addr2Host( m_sh.sh_addr, m_pIELFI->GetEncoding() );
}


Elf32_Word
ELFISection::GetSize() const
{
    return Convert32Word2Host( m_sh.sh_size, m_pIELFI->GetEncoding() );
}

Elf32_Off
ELFISection::GetOffset() const
{
    return Convert32Off2Host( m_sh.sh_offset, m_pIELFI->GetEncoding() );
}


Elf32_Word
ELFISection::GetLink() const
{
    return Convert32Word2Host( m_sh.sh_link, m_pIELFI->GetEncoding() );
}


Elf32_Word
ELFISection::GetInfo() const
{
    return Convert32Word2Host( m_sh.sh_info, m_pIELFI->GetEncoding() );
}


Elf32_Word
ELFISection::GetAddrAlign() const
{
    return Convert32Word2Host( m_sh.sh_addralign, m_pIELFI->GetEncoding() );
}


Elf32_Word
ELFISection::GetEntrySize() const
{
    return Convert32Word2Host( m_sh.sh_entsize, m_pIELFI->GetEncoding() );
}


const char*
ELFISection::GetData() const
{
    Elf32_Word size = GetSize();
    if ( 0 == m_data && SHT_NULL != GetType() && SHT_NOBITS != GetType() &&
         0 != size ) {
        m_data = new char[size];
        if ( 0 != m_data ) {
            m_pStream->seekg( Convert32Off2Host( m_sh.sh_offset,
                              m_pIELFI->GetEncoding() ) + m_nFileOffset );
            m_pStream->read( m_data, size );
        }
    }

    return m_data;
}
