/*
ELFISegment.cpp - ELF segment reader implementation.
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


IELFISegment::~IELFISegment()
{
}


ELFISegment::ELFISegment( IELFI* pIELFI, std::istream* pStream, int nFileOffset,
                          Elf32_Phdr* pHeader, Elf32_Half index ) :
    m_pIELFI( pIELFI ), m_pStream( pStream ), m_nFileOffset( nFileOffset ), m_index( index )
{
    std::fill_n( reinterpret_cast<char*>( &m_sh ), sizeof( m_sh ), '\0' );
    m_sh      = *pHeader;
    m_data    = 0;
}


ELFISegment::~ELFISegment()
{
    delete [] m_data;
}


int
ELFISegment::AddRef() const
{
    return m_pIELFI->AddRef();
}


int
ELFISegment::Release() const
{
    return m_pIELFI->Release();
}


Elf32_Word
ELFISegment::GetType() const
{
    return Convert32Word2Host( m_sh.p_type, m_pIELFI->GetEncoding() );
}


Elf32_Addr
ELFISegment::GetVirtualAddress() const
{
    return Convert32Addr2Host( m_sh.p_vaddr, m_pIELFI->GetEncoding() );
}


Elf32_Addr
ELFISegment::GetPhysicalAddress() const
{
    return Convert32Addr2Host( m_sh.p_paddr, m_pIELFI->GetEncoding() );
}


Elf32_Word
ELFISegment::GetFileSize() const
{
    return Convert32Word2Host( m_sh.p_filesz, m_pIELFI->GetEncoding() );
}


Elf32_Word
ELFISegment::GetMemSize() const
{
    return Convert32Word2Host( m_sh.p_memsz, m_pIELFI->GetEncoding() );
}


Elf32_Word
ELFISegment::GetFlags() const
{
    return Convert32Word2Host( m_sh.p_flags, m_pIELFI->GetEncoding() );
}


Elf32_Word
ELFISegment::GetAlign() const
{
    return Convert32Word2Host( m_sh.p_align, m_pIELFI->GetEncoding() );
}


const char*
ELFISegment::GetData() const
{
    if ( 0 == m_data && SHT_NULL != GetType() && SHT_NOBITS != GetType() &&
         0 != GetFileSize() ) {
        m_pStream->seekg( Convert32Off2Host( m_sh.p_offset,
                          m_pIELFI->GetEncoding() ) + m_nFileOffset );
        Elf32_Word size = GetFileSize();
        m_data = new char[size];
        if ( 0 != m_data ) {
            m_pStream->read( m_data, size );
        }
    }

    return m_data;
}
