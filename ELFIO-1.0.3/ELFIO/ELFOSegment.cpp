/*
ELFOSegment.cpp - ELF segment producer implementation.
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


ELFOSegment::ELFOSegment( IELFO* pIELFO, Elf32_Word type, Elf32_Addr vaddr,
                          Elf32_Addr paddr, Elf32_Word flags, Elf32_Word align ) :
        m_pIELFO( pIELFO )
{
    std::fill_n( reinterpret_cast<char*>( &m_ph ), sizeof( m_ph ), '\0' );
    m_ph.p_type  = Convert32Word2Host( type,  m_pIELFO->GetEncoding() );
    m_ph.p_vaddr = Convert32Addr2Host( vaddr, m_pIELFO->GetEncoding() );
    m_ph.p_paddr = Convert32Addr2Host( paddr, m_pIELFO->GetEncoding() );
    m_ph.p_flags = Convert32Word2Host( flags, m_pIELFO->GetEncoding() );
    m_ph.p_align = Convert32Word2Host( align, m_pIELFO->GetEncoding() );
}


int
ELFOSegment::AddRef()
{
    return m_pIELFO->AddRef();
}


int
ELFOSegment::Release()
{
    return m_pIELFO->Release();
}


Elf32_Word
ELFOSegment::GetType() const
{
    return Convert32Word2Host( m_ph.p_type, m_pIELFO->GetEncoding() );
}


Elf32_Word
ELFOSegment::GetFlags() const
{
    return Convert32Word2Host( m_ph.p_flags, m_pIELFO->GetEncoding() );
}


Elf32_Word
ELFOSegment::GetAlign() const
{
    return Convert32Word2Host( m_ph.p_align, m_pIELFO->GetEncoding() );
}


Elf32_Addr
ELFOSegment::GetVirtualAddress() const
{
    return Convert32Word2Host( m_ph.p_vaddr, m_pIELFO->GetEncoding() );
}


Elf32_Addr
ELFOSegment::GetPhysicalAddress() const
{
    return Convert32Word2Host( m_ph.p_paddr, m_pIELFO->GetEncoding() );
}


void
ELFOSegment::SetAddresses( Elf32_Addr vaddr, Elf32_Addr paddr )
{
    m_ph.p_vaddr = Convert32Addr2Host( vaddr, m_pIELFO->GetEncoding() );
    m_ph.p_paddr = Convert32Addr2Host( paddr, m_pIELFO->GetEncoding() );

    // Change section addresses acordingly
    Elf32_Addr addr = GetPhysicalAddress();
    std::vector<IELFOSection*>::const_iterator it;
    for ( it = m_sections.begin(); it != m_sections.end(); ++it ) {
        (*it)->SetAddress( addr );
        addr += (*it)->GetSize();
    }
}


Elf32_Word
ELFOSegment::GetFileSize() const
{
    Elf32_Word size = 0;
    if ( !m_sections.empty() ) {
        std::vector<IELFOSection*>::const_iterator it;
        it   = m_sections.end() - 1;
        size = (Elf32_Word)m_pIELFO->GetSectionFileOffset( (*it)->GetIndex() ) +
               (*it)->GetSize();
    }

    return size;
}


Elf32_Word
ELFOSegment::GetMemSize() const
{
    Elf32_Word size = GetFileSize();

    std::vector<IELFOSection*>::const_iterator it;
    for ( it = m_sections.begin(); it != m_sections.end(); ++it ) {
        if ( (*it)->GetType() == SHT_NOBITS || (*it)->GetType() == SHT_NULL ) {
            size += (*it)->GetSize();
        }
    }

    return size;
}


Elf32_Half
ELFOSegment::AddSection( IELFOSection* pSection )
{
    if ( 0 != pSection ) {
        // Count prev. section sizes
        Elf32_Addr addr = GetPhysicalAddress() + GetMemSize();
        pSection->SetAddress( addr );
        m_sections.push_back( pSection );
        if ( pSection->GetAddrAlign() > GetAlign() ) {
            m_ph.p_align = Convert32Word2Host( pSection->GetAddrAlign(),
                                               m_pIELFO->GetEncoding() );
        }
    }

    return m_sections.size();
}


ELFIO_Err
ELFOSegment::Save( std::ofstream& f, std::streampos posHeader )
{
    Elf32_Word addSize = 0;

    if ( !m_sections.empty() ) {
        Elf32_Word off = m_pIELFO->GetSectionFileOffset(
                                       m_sections[0]->GetIndex() );
        Elf32_Word align = GetAlign() ? GetAlign() : 1;
        addSize = off - ( off / align ) * align;
        m_ph.p_offset = Convert32Off2Host( ( off / align ) * align, m_pIELFO->GetEncoding() );

        // Adjust sections addresses
        std::vector<IELFOSection*>::const_iterator it;
        for ( it = m_sections.begin(); it != m_sections.end(); ++it ) {
            (*it)->SetAddress( (*it)->GetAddress() + addSize );

            if ( (*it)->GetName() == ".text" ) {
                m_pIELFO->SetEntry( m_pIELFO->GetEntry() + addSize );
            }
        }
    }

    m_ph.p_filesz = Convert32Word2Host( GetFileSize() + addSize,
                                        m_pIELFO->GetEncoding() );
    m_ph.p_memsz = Convert32Word2Host( GetMemSize() + addSize,
                                       m_pIELFO->GetEncoding() );

    f.seekp( posHeader );
    f.write( reinterpret_cast<const char*>( &m_ph ), sizeof( Elf32_Phdr ) );

    return ERR_ELFIO_NO_ERROR;
}
