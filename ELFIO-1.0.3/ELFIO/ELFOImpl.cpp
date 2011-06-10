/*
ELFOImpl.cpp - Implementation of the producer class.
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
#include <fstream>
#include "ELFOImpl.h"
#include "ELFIOUtils.h"


ELFO::ELFO()
{
    m_nRefCnt = 1;
    std::fill_n( reinterpret_cast<char*>( &m_header ), sizeof( m_header ), '\0' );
}


ELFO::~ELFO()
{
    std::vector<ELFOSection*>::iterator it;
    for ( it = m_sections.begin(); it != m_sections.end(); ++it ) {
        delete (*it);
    }

    std::vector<ELFOSegment*>::iterator itSeg;
    for ( itSeg = m_segments.begin(); itSeg != m_segments.end(); ++itSeg ) {
        delete (*itSeg);
    }
}


int
ELFO::AddRef()
{
    return ++m_nRefCnt;
}


int
ELFO::Release()
{
    int nRet = --m_nRefCnt;
    if ( 0 == m_nRefCnt ) {
        delete this;
    }

    return nRet;
}


ELFIO_Err
ELFO::Save( const std::string& sFileName )
{
    ELFIO_Err err = ERR_ELFIO_CANT_OPEN;
    std::ofstream f( sFileName.c_str(), std::ios::out | std::ios::binary );
    if ( f ) {
        // Fill not completed fields in the header
        if ( 0 != GetSegmentNum() ) {
            m_header.e_phoff = Convert32Off2Host( sizeof( Elf32_Ehdr ), GetEncoding() );
        }
        m_header.e_shoff = Convert32Off2Host( sizeof( Elf32_Ehdr ) +
                               sizeof(Elf32_Phdr) * GetSegmentNum(), GetEncoding() );
        m_header.e_phnum = Convert32Half2Host( GetSegmentNum(), GetEncoding() );
        m_header.e_shnum = Convert32Half2Host( GetSectionsNum(), GetEncoding() );

        // Write program segments
        std::streampos posHeader = Convert32Off2Host( m_header.e_phoff, GetEncoding() );
        unsigned int i;
        for ( i = 0; i < m_segments.size(); ++i ) {
            m_segments[i]->Save( f, posHeader );
            posHeader += sizeof( Elf32_Phdr );
        }

        // Write sections and their data
        std::streampos posHeaderStart  = Convert32Off2Host( m_header.e_shoff, GetEncoding() );
        for ( i = 0; i < m_sections.size(); ++i ) {
            m_sections[i]->Save( f, posHeaderStart, GetSectionFileOffset( i ) );
            posHeaderStart  += sizeof( Elf32_Shdr );
        }

        // Write the header
        f.seekp( 0 );
        f.write( reinterpret_cast<const char*>( &m_header ), sizeof( Elf32_Ehdr ) );

        f.close();
        err = ERR_ELFIO_NO_ERROR;
    }

    return err;
}


ELFIO_Err
ELFO::SetAttr( unsigned char fileClass,
               unsigned char encoding,
               unsigned char ELFVersion,
               Elf32_Half    type,
               Elf32_Half    machine,
               Elf32_Word    version,
               Elf32_Word    flags )
{
    m_header.e_ident[EI_MAG0]    = ELFMAG0;
    m_header.e_ident[EI_MAG1]    = ELFMAG1;
    m_header.e_ident[EI_MAG2]    = ELFMAG2;
    m_header.e_ident[EI_MAG3]    = ELFMAG3;
    m_header.e_ident[EI_CLASS]   = fileClass;
    m_header.e_ident[EI_DATA]    = encoding;
    m_header.e_ident[EI_VERSION] = ELFVersion;

    m_header.e_type    = Convert32Half2Host( type, encoding );
    m_header.e_machine = Convert32Half2Host( machine, encoding );
    m_header.e_version = Convert32Word2Host( version, encoding );
    m_header.e_flags   = Convert32Word2Host( flags, encoding );

    m_header.e_ehsize    = Convert32Half2Host( sizeof( m_header ), encoding );
    m_header.e_phentsize = Convert32Half2Host( sizeof( Elf32_Phdr ), encoding );
    m_header.e_shentsize = Convert32Half2Host( sizeof( Elf32_Shdr ), encoding );
    m_header.e_shstrndx  = Convert32Half2Host( 1, encoding );

    // Create empty and section header string table sections
    ELFOSection* pSec0 = new ELFOSection( 0, this, "", 0, 0, 0, 0, 0 );
    m_sections.push_back( pSec0 );
    pSec0->SetNameIndex( 0 );
    ELFOSection* pshstrtab = new ELFOSection( 1, this, ".shstrtab", SHT_STRTAB, 0, 0, 0, 0 );
    m_sections.push_back( pshstrtab );

    // Add the name to the section header string table
    IELFOStringWriter* pStrTbl;
    if ( CreateSectionWriter( ELFO_STRING, pshstrtab,
                              reinterpret_cast<void**>( &pStrTbl ) ) ==
             ERR_ELFIO_NO_ERROR ) {
        Elf32_Word index = pStrTbl->AddString( pshstrtab->GetName().c_str() );
        pshstrtab->SetNameIndex( index );
        pStrTbl->Release();
    }

    return ERR_ELFIO_NO_ERROR;
}


Elf32_Addr
ELFO::GetEntry() const
{
     return Convert32Addr2Host( m_header.e_entry, GetEncoding() );
}


ELFIO_Err
ELFO::SetEntry( Elf32_Addr entry )
{
    m_header.e_entry = Convert32Addr2Host( entry, GetEncoding() );

    return ERR_ELFIO_NO_ERROR;
}


unsigned char
ELFO::GetEncoding() const
{
    return m_header.e_ident[EI_DATA];
}


Elf32_Half
ELFO::GetSectionsNum() const
{
    return m_sections.size();
}


IELFOSection*
ELFO::GetSection( Elf32_Half index ) const
{
    IELFOSection* pRet = 0;
    if ( index < GetSectionsNum() ) {
        pRet = m_sections[index];
        pRet->AddRef();
    }

    return pRet;
}


IELFOSection*
ELFO::GetSection( const std::string& name ) const
{
    IELFOSection* pRet = 0;

    std::vector<ELFOSection*>::const_iterator it;
    for ( it = m_sections.begin(); it != m_sections.end(); ++it ) {
        if ( (*it)->GetName() == name ) {
            pRet = *it;
            pRet->AddRef();
            break;
        }
    }

    return pRet;
}


IELFOSection*
ELFO::AddSection( const std::string& name,
                  Elf32_Word type,
                  Elf32_Word flags,
                  Elf32_Word info,
                  Elf32_Word addrAlign,
                  Elf32_Word entrySize )
{
    ELFOSection* pSec = new ELFOSection( m_sections.size(), this, name, type,
                                         flags, info, addrAlign, entrySize );
    if ( 0 != pSec ) {
        pSec->AddRef();
        m_sections.push_back( pSec );

        // Add the name to the section header string table
        IELFOSection* pshstrtab = GetSection( 1 );
        IELFOStringWriter* pStrTbl;
        if ( CreateSectionWriter( ELFO_STRING, pshstrtab,
                                  reinterpret_cast<void**>( &pStrTbl ) ) ==
                 ERR_ELFIO_NO_ERROR ) {
            Elf32_Word index = pStrTbl->AddString( name.c_str() );
            pSec->SetNameIndex( index );
            pStrTbl->Release();
        }
        pshstrtab->Release();
    }

    return pSec;
}


std::streampos
ELFO::GetSectionFileOffset( Elf32_Half index ) const
{
    std::streampos nRet = sizeof( Elf32_Ehdr ) +
                          sizeof( Elf32_Phdr ) * GetSegmentNum() +
                          sizeof( Elf32_Shdr ) * GetSectionsNum();
    Elf32_Half size     = m_sections.size();
    Elf32_Word secAlign = 0;
    for ( Elf32_Half i = 0; i < size && i < index; ++i ) {
        if ( m_sections[i]->GetType() != SHT_NOBITS && m_sections[i]->GetType() != SHT_NULL ) {
            secAlign = m_sections[i]->GetAddrAlign();
            if ( secAlign > 1 && nRet % secAlign != 0 ) {
                nRet += secAlign - nRet % secAlign;
            }
            nRet += m_sections[i]->GetSize();
        }
    }
    if ( m_sections[index]->GetType() != SHT_NOBITS && m_sections[index]->GetType() != SHT_NULL ) {
        secAlign = m_sections[index]->GetAddrAlign();
        if ( secAlign > 1 && nRet % secAlign != 0 ) {
            nRet += secAlign - nRet % secAlign;
        }
    }

    return nRet;
}


Elf32_Half
ELFO::GetSegmentNum() const
{
    return m_segments.size();
}


IELFOSegment*
ELFO::AddSegment( Elf32_Word type,  Elf32_Addr vaddr,
                  Elf32_Addr paddr, Elf32_Word flags, Elf32_Word align )
{
    ELFOSegment* pSeg = new ELFOSegment( this, type, vaddr, paddr, flags, align );
    if ( 0 != pSeg ) {
        pSeg->AddRef();
        m_segments.push_back( pSeg );
    }

    return pSeg;
}


IELFOSegment*
ELFO::GetSegment( Elf32_Half index ) const
{
    IELFOSegment* pRet = 0;
    if ( index < GetSegmentNum() ) {
        pRet = m_segments[index];
        pRet->AddRef();
    }

    return pRet;
}


ELFIO_Err
ELFO::CreateSectionWriter( WriterType type, IELFOSection* pSection,
                           void** ppObj ) const
{
    ELFIO_Err eRet = ERR_ELFIO_NO_ERROR;

    switch ( type ) {
    case ELFO_STRING:
        *ppObj = new ELFOStringWriter( const_cast<ELFO*>( this ), pSection );
        break;
    case ELFO_RELOCATION:
        *ppObj = new ELFORelocationTable( const_cast<ELFO*>( this ), pSection );
        break;
    case ELFO_SYMBOL:
        *ppObj = new ELFOSymbolTable( const_cast<ELFO*>( this ), pSection );
        break;
    case ELFO_NOTE:
        *ppObj = new ELFONotesWriter( const_cast<ELFO*>( this ), pSection );
        break;
    default:
        eRet  = ERR_NO_SUCH_READER;
        ppObj = 0;
    }
    return eRet;
}
