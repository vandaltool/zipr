/*
ELFOSection.cpp - ELF section producer implementation.
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


#include <algorithm>
#include "ELFOImpl.h"
#include "ELFIOUtils.h"


ELFOSection::ELFOSection( Elf32_Half index,
                          IELFO*     pIELFO,
                          const std::string& name,
                          Elf32_Word type,
                          Elf32_Word flags,
                          Elf32_Word info,
                          Elf32_Word addrAlign,
                          Elf32_Word entrySize ) :
        m_index( index ),
        m_pIELFO( pIELFO ),
        m_name( name ),
        m_pData( 0 )
{
    std::fill_n( reinterpret_cast<char*>( &m_sh ), sizeof( m_sh ), '\0' );
    m_sh.sh_type      = Convert32Word2Host( type, m_pIELFO->GetEncoding() );
    m_sh.sh_flags     = Convert32Word2Host( flags, m_pIELFO->GetEncoding() );
    m_sh.sh_info      = Convert32Word2Host( info, m_pIELFO->GetEncoding() );
    m_sh.sh_addralign = Convert32Word2Host( addrAlign, m_pIELFO->GetEncoding() );
    m_sh.sh_entsize   = Convert32Word2Host( entrySize, m_pIELFO->GetEncoding() );
}


ELFOSection::~ELFOSection()
{
    delete [] m_pData;
}


int
ELFOSection::AddRef()
{
    return m_pIELFO->AddRef();
}


int
ELFOSection::Release()
{
    return m_pIELFO->Release();
}

Elf32_Half
ELFOSection::GetIndex() const
{
    return m_index;
}


std::string
ELFOSection::GetName() const
{
    return m_name;
}


Elf32_Word
ELFOSection::GetNameIndex() const
{
    return Convert32Word2Host( m_sh.sh_name, m_pIELFO->GetEncoding() );
}


void
ELFOSection::SetNameIndex( Elf32_Word index )
{
    m_sh.sh_name = Convert32Word2Host( index, m_pIELFO->GetEncoding() );
}


Elf32_Word
ELFOSection::GetType() const
{
    return Convert32Word2Host( m_sh.sh_type, m_pIELFO->GetEncoding() );
}


Elf32_Word
ELFOSection::GetFlags() const
{
    return Convert32Word2Host( m_sh.sh_flags, m_pIELFO->GetEncoding() );
}


Elf32_Word
ELFOSection::GetInfo() const
{
    return Convert32Word2Host( m_sh.sh_info, m_pIELFO->GetEncoding() );
}


Elf32_Word
ELFOSection::GetAddrAlign() const
{
    return Convert32Word2Host( m_sh.sh_addralign, m_pIELFO->GetEncoding() );
}


Elf32_Word
ELFOSection::GetEntrySize() const
{
    return Convert32Word2Host( m_sh.sh_entsize, m_pIELFO->GetEncoding() );
}


Elf32_Addr
ELFOSection::GetAddress() const
{
    return Convert32Word2Host( m_sh.sh_addr, m_pIELFO->GetEncoding() );
}


void
ELFOSection::SetAddress( Elf32_Addr addr )
{
    m_sh.sh_addr = Convert32Word2Host( addr, m_pIELFO->GetEncoding() );
}


Elf32_Word
ELFOSection::GetLink() const
{
    return Convert32Word2Host( m_sh.sh_link, m_pIELFO->GetEncoding() );
}


void
ELFOSection::SetLink( Elf32_Word link )
{
    m_sh.sh_link = Convert32Word2Host( link, m_pIELFO->GetEncoding() );
}


char*
ELFOSection::GetData() const
{
    return m_pData;
}


Elf32_Word
ELFOSection::GetSize() const
{
    return Convert32Word2Host( m_sh.sh_size, m_pIELFO->GetEncoding() );
}


ELFIO_Err
ELFOSection::SetData( const char* pData, Elf32_Word size )
{
    ELFIO_Err err = ERR_ELFIO_MEMORY;

    if ( GetType() != SHT_NOBITS ) {
        delete [] m_pData;
        m_pData = new char[size];
        if ( 0 != m_pData && 0 != pData && 0 != size ) {
            std::copy( pData, pData + size, m_pData );
            err = ERR_ELFIO_NO_ERROR;
        }
    }
    else {
        err = ERR_ELFIO_NO_ERROR;
    }

    m_sh.sh_size = Convert32Word2Host( size, m_pIELFO->GetEncoding() );

    return err;
}


ELFIO_Err
ELFOSection::SetData( const std::string& data )
{
    return SetData( data.c_str(), data.size() );
}


ELFIO_Err
ELFOSection::AddData( const char* pData, Elf32_Word size )
{
    ELFIO_Err err = ERR_ELFIO_MEMORY;

    if ( GetType() != SHT_NOBITS ) {
        char* pNewData = new char[GetSize() + size];
        if ( 0 != pNewData ) {
            std::copy( m_pData, m_pData + GetSize(), pNewData );
            std::copy( pData, pData + size, pNewData + GetSize() );
            delete [] m_pData;
            m_pData = pNewData;
            m_sh.sh_size = Convert32Word2Host( GetSize() + size,
                                               m_pIELFO->GetEncoding() );
            err = ERR_ELFIO_NO_ERROR;
        }
    }

    return err;
}


ELFIO_Err
ELFOSection::AddData( const std::string& data )
{
    return AddData( data.c_str(), data.size() );
}


ELFIO_Err
ELFOSection::Save( std::ofstream& f, std::streampos posHeader,
                   std::streampos posData )
{
    if ( 0 != GetIndex() && SHT_NOBITS != GetType() ) {
        m_sh.sh_offset = Convert32Off2Host( posData, m_pIELFO->GetEncoding() );
    }
    f.seekp( posHeader );
    f.write( reinterpret_cast<const char*>( &m_sh ), sizeof( Elf32_Shdr ) );

    if ( SHT_NOBITS != GetType() ) {
        f.seekp( posData );
        f.write( GetData(), GetSize() );
    }

    return ERR_ELFIO_NO_ERROR;
}
