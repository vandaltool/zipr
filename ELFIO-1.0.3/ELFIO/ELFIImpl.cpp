/*
ELFIImpl.cpp - Implementation of the reader class.
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

#include <iosfwd>
#include <algorithm>
#include "ELFIImpl.h"
#include "ELFIOUtils.h"


ELFI::ELFI()
{
    m_nRefCnt      = 1;
    m_nFileOffset  = 0;
    m_bOwnStream   = false;
    m_bInitialized = false;
    std::fill_n( reinterpret_cast<char*>( &m_header ), sizeof( m_header ), '\0' );
}


ELFI::~ELFI()
{
    std::vector<const IELFISection*>::const_iterator it;
    for ( it = m_sections.begin(); it != m_sections.end(); ++it ) {
        delete const_cast<IELFISection*>( *it );
    }
    std::vector<const IELFISegment*>::const_iterator it1;
    for ( it1 = m_segments.begin(); it1 != m_segments.end(); ++it1 ) {
        delete const_cast<IELFISegment*>( *it1 );
    }

    if ( m_bOwnStream ) {
        ((std::ifstream*)m_pStream)->close();
        delete m_pStream;
    }
}


ELFIO_Err
ELFI::Load( const std::string& sFileName )
{
    if ( IsInitialized() ) {         // Already opened?
        return ERR_ELFIO_INITIALIZED;
    }

    // Open binary file
    std::ifstream* pS = new std::ifstream;
    if ( 0 == pS ) {
        return ERR_ELFIO_MEMORY;
    }
    pS->open( sFileName.c_str(), std::ios::in | std::ios::binary );
    if ( !*pS ) {
        return ERR_ELFIO_CANT_OPEN;
    }

    ELFIO_Err err = Load( pS, 0 );

    m_bOwnStream = true;

    return err;
}


ELFIO_Err
ELFI::Load( std::istream* pStream, int startPos )
{
    if ( IsInitialized() ) {         // Already opened?
        return ERR_ELFIO_INITIALIZED;
    }

    m_pStream     = pStream;
    m_bOwnStream  = false;
    m_nFileOffset = startPos;

    // Read ELF header
    m_pStream->seekg( m_nFileOffset );
    m_pStream->read( reinterpret_cast<char*>( &m_header ), sizeof( m_header ) );

    // Is it ELF file?
    if ( m_pStream->gcount() != sizeof( m_header ) ||
         m_header.e_ident[EI_MAG0] != ELFMAG0    ||
         m_header.e_ident[EI_MAG1] != ELFMAG1    ||
         m_header.e_ident[EI_MAG2] != ELFMAG2    ||
         m_header.e_ident[EI_MAG3] != ELFMAG3 ) {

        return ERR_ELFIO_NOT_ELF;
    }

    ELFIO_Err nRet = LoadSections();
    if ( ERR_ELFIO_NO_ERROR != nRet ) {
        return nRet;
    }

    nRet = LoadSegments();
    if ( ERR_ELFIO_NO_ERROR != nRet ) {
        return nRet;
    }

    m_bInitialized = true;

    return ERR_ELFIO_NO_ERROR;
}


ELFIO_Err
ELFI::LoadSections()
{
    Elf32_Shdr buffer;
    Elf32_Half nEntrySize = Convert32Half2Host( m_header.e_shentsize, GetEncoding() );
    Elf32_Half nNum       = Convert32Half2Host( m_header.e_shnum, GetEncoding() );
    Elf32_Off  nOff       = Convert32Off2Host( m_header.e_shoff, GetEncoding() );

#ifdef _MSC_VER
    unsigned int buflen = std::_MIN( sizeof( Elf32_Shdr ), (size_t)nEntrySize );
#else
    unsigned int buflen = std::min( sizeof( Elf32_Shdr ), (std::size_t)nEntrySize );
#endif

    for ( int i = 0; i < nNum; ++i ) {
        m_pStream->seekg( nOff + i * nEntrySize + m_nFileOffset );
        m_pStream->read( reinterpret_cast<char*>( &buffer ), buflen );
        ELFISection* pSec = new ELFISection( this, m_pStream, m_nFileOffset, &buffer, i );

        // Add section into the section container
        m_sections.push_back( pSec );
    }

    return ERR_ELFIO_NO_ERROR;
}


ELFIO_Err
ELFI::LoadSegments()
{
    Elf32_Phdr buffer;
    Elf32_Half nEntrySize = Convert32Half2Host( m_header.e_phentsize, GetEncoding() );
    Elf32_Half nNum       = Convert32Half2Host( m_header.e_phnum, GetEncoding() );
    Elf32_Off  nOff       = Convert32Off2Host( m_header.e_phoff, GetEncoding() );

#ifdef _MSC_VER
    int buflen = std::_MIN( sizeof( Elf32_Phdr ), (size_t)nEntrySize );
#else
    int buflen = std::min( sizeof( Elf32_Phdr ), (std::size_t)nEntrySize );
#endif

    for ( int i = 0; i < nNum; ++i ) {
        m_pStream->seekg( nOff + i * nEntrySize + m_nFileOffset );
        m_pStream->read( reinterpret_cast<char*>( &buffer ), buflen );
        ELFISegment* pSeg = new ELFISegment( this, m_pStream, m_nFileOffset, &buffer, i );

        // Add section into the section container
        m_segments.push_back( pSeg );
    }

    return ERR_ELFIO_NO_ERROR;
}


bool
ELFI::IsInitialized() const
{
    return m_bInitialized;
}


int
ELFI::AddRef() const
{
    return ++m_nRefCnt;
}


int
ELFI::Release() const
{
    int nRet = --m_nRefCnt;
    if ( 0 == m_nRefCnt ) {
        delete this;
    }

    return nRet;
}


unsigned char
ELFI::GetClass() const
{
    return m_header.e_ident[EI_CLASS];
}


unsigned char
ELFI::GetEncoding() const
{
    return m_header.e_ident[EI_DATA];
}


unsigned char
ELFI::GetELFVersion() const
{
    return m_header.e_ident[EI_VERSION];
}


Elf32_Half
ELFI::GetType() const
{
    return Convert32Half2Host( m_header.e_type, GetEncoding() );
}


Elf32_Half
ELFI::GetMachine() const
{
    return Convert32Half2Host( m_header.e_machine, GetEncoding() );
}


Elf32_Word
ELFI::GetVersion() const
{
    return Convert32Word2Host( m_header.e_version, GetEncoding() );
}


Elf32_Addr
ELFI::GetEntry() const
{
    return Convert32Addr2Host( m_header.e_entry, GetEncoding() );
}


Elf32_Word
ELFI::GetFlags() const
{
    return Convert32Word2Host( m_header.e_flags, GetEncoding() );
}


Elf32_Half
ELFI::GetSecStrNdx() const
{
    return Convert32Half2Host( m_header.e_shstrndx, GetEncoding() );
}


Elf32_Half
ELFI::GetSectionsNum() const
{
    return m_sections.size();
}


const IELFISection*
ELFI::GetSection( Elf32_Half index ) const
{
    if ( index >= GetSectionsNum() ) {
        return 0;
    }

    m_sections[index]->AddRef();
    return m_sections[index];
}


const IELFISection*
ELFI::GetSection( const std::string& name ) const
{
    const IELFISection* pRet = 0;
    std::vector<const IELFISection*>::const_iterator it;
    for ( it = m_sections.begin(); it != m_sections.end(); ++it ) {
        if ( (*it)->GetName() == name ) {
            pRet = *it;
            pRet->AddRef();
            break;
        }
    }

    return pRet;
}


Elf32_Half
ELFI::GetSegmentsNum() const
{
    return m_segments.size();
}


const IELFISegment*
ELFI::GetSegment( Elf32_Half index ) const
{
    if ( index >= GetSegmentsNum() ) {
        return 0;
    }
    m_segments[index]->AddRef();
    return m_segments[index];
}


ELFIO_Err
ELFI::CreateSectionReader( ReaderType type, const IELFISection* pSection,
                           void** ppObj ) const
{
    ELFIO_Err eRet = ERR_ELFIO_NO_ERROR;

    switch ( type ) {
    case ELFI_STRING:
        *ppObj = (IELFIStringReader*)( new ELFIStringReader( this, pSection ) );
        break;
    case ELFI_SYMBOL:
        *ppObj = (IELFISymbolTable*)( new ELFISymbolTable( this, pSection ) );
        break;
    case ELFI_RELOCATION:
        *ppObj = (IELFIRelocationTable*)( new ELFIRelocationTable( this, pSection ) );
        break;
    case ELFI_NOTE:
        *ppObj = (IELFINoteReader*)( new ELFINoteReader( this, pSection ) );
        break;
    case ELFI_DYNAMIC:
        *ppObj = (IELFIDynamicReader*)( new ELFIDynamicReader( this, pSection ) );
        break;
    default:
        eRet  = ERR_NO_SUCH_READER;
        ppObj = 0;
    }
    return eRet;
}


ELFIReaderImpl::ELFIReaderImpl( const IELFI* pIELFI, const IELFISection* pSection )
{
    m_nRefCnt  = 1;
    m_pIELFI   = pIELFI;
    m_pSection = pSection;
    m_pIELFI->AddRef();
    m_pSection->AddRef();
}


ELFIReaderImpl::~ELFIReaderImpl()
{
}


int
ELFIReaderImpl::AddRef() const
{
    m_pIELFI->AddRef();
    m_pSection->AddRef();
    return ++m_nRefCnt;
}


int
ELFIReaderImpl::Release() const
{
    m_pSection->Release();
    m_pIELFI->Release();

    int nRet = --m_nRefCnt;
    if ( 0 == m_nRefCnt ) {
        delete this;
    }

    return nRet;
}


Elf32_Half
ELFIReaderImpl::GetIndex() const
{
    return m_pSection->GetIndex();
}


std::string
ELFIReaderImpl::GetName() const
{
    return m_pSection->GetName();
}


Elf32_Word
ELFIReaderImpl::GetType() const
{
    return m_pSection->GetType();
}


Elf32_Word
ELFIReaderImpl::GetFlags() const
{
    return m_pSection->GetFlags();
}


Elf32_Addr
ELFIReaderImpl::GetAddress() const
{
    return m_pSection->GetAddress();
}


Elf32_Word
ELFIReaderImpl::GetSize() const
{
    return m_pSection->GetSize();
}

Elf32_Off
ELFIReaderImpl::GetOffset() const
{
    return m_pSection->GetOffset();
}



Elf32_Word
ELFIReaderImpl::GetLink() const
{
    return m_pSection->GetLink();
}


Elf32_Word
ELFIReaderImpl::GetInfo() const
{
    return m_pSection->GetInfo();
}


Elf32_Word
ELFIReaderImpl::GetAddrAlign() const
{
    return m_pSection->GetAddrAlign();
}


Elf32_Word
ELFIReaderImpl::GetEntrySize() const
{
    return m_pSection->GetEntrySize();
}


const char*
ELFIReaderImpl::GetData() const
{
    return m_pSection->GetData();
}
