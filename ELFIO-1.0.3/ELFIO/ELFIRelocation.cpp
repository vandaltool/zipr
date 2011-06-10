/*
ELFIRelocation.cpp - ELF relocation table reader implementation.
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


ELFIRelocationTable::ELFIRelocationTable( const IELFI* pIELFI, const IELFISection* pSection ) :
    ELFIReaderImpl( pIELFI, pSection )
{
    const IELFISection* pSymSection = m_pIELFI->GetSection( GetSymbolTableIndex() );
    m_pIELFI->CreateSectionReader( IELFI::ELFI_SYMBOL, pSymSection,
                                   (void**)&m_pSymTbl );
    pSymSection->Release();
}


ELFIRelocationTable::~ELFIRelocationTable()
{
}


int
ELFIRelocationTable::AddRef() const
{
    m_pSymTbl->AddRef();

    return ELFIReaderImpl::AddRef();
}


int
ELFIRelocationTable::Release() const
{
    m_pSymTbl->Release();

    return ELFIReaderImpl::Release();
}


Elf32_Half
ELFIRelocationTable::GetSymbolTableIndex() const
{
    return (Elf32_Half)m_pSection->GetLink();
}


Elf32_Half
ELFIRelocationTable::GetTargetSectionIndex() const
{
    return (Elf32_Half)m_pSection->GetInfo();
}


Elf32_Word
ELFIRelocationTable::GetEntriesNum() const
{
    Elf32_Word nRet = 0;
    if ( 0 != m_pSection->GetEntrySize() ) {
        nRet = m_pSection->GetSize() / m_pSection->GetEntrySize();
    }
    
    return nRet;
}


ELFIO_Err
ELFIRelocationTable::GetEntry( Elf32_Word     index,
                               Elf32_Addr&    offset, 
                               Elf32_Word&    symbol,
                               unsigned char& type,
                               Elf32_Sword&   addend ) const
{
    ELFIO_Err nRet = ERR_ELFIO_INDEX_ERROR;
    
    if ( index < GetEntriesNum() ) {    // Is index valid
        if ( SHT_REL == m_pSection->GetType() ) {
            const Elf32_Rel* pEntry = reinterpret_cast<const Elf32_Rel*>(
                    m_pSection->GetData() + index * m_pSection->GetEntrySize() );
            offset = Convert32Addr2Host( pEntry->r_offset, m_pIELFI->GetEncoding() );
            Elf32_Word tmp = Convert32Word2Host( pEntry->r_info, m_pIELFI->GetEncoding() );
            symbol = tmp >> 8;
            type   = (unsigned char)tmp;
            addend = 0;

            nRet = ERR_ELFIO_NO_ERROR;
        }
        else if ( SHT_RELA == m_pSection->GetType() ) {
            const Elf32_Rela* pEntry = reinterpret_cast<const Elf32_Rela*>(
                    m_pSection->GetData() + index * m_pSection->GetEntrySize() );
            offset = Convert32Addr2Host( pEntry->r_offset, m_pIELFI->GetEncoding() );
            Elf32_Word tmp = Convert32Word2Host( pEntry->r_info, m_pIELFI->GetEncoding() );
            symbol = tmp >> 8;
            type   = (unsigned char)tmp;
            addend = Convert32Sword2Host( pEntry->r_addend, m_pIELFI->GetEncoding() );

            nRet = ERR_ELFIO_NO_ERROR;
        }
    }    

    return nRet;
}


ELFIO_Err
ELFIRelocationTable::GetEntry( Elf32_Word     index,
                               Elf32_Addr&    offset, 
                               Elf32_Addr&    symbolValue,
                               std::string&   symbolName,
                               unsigned char& type,
                               Elf32_Sword&   addend,
                               Elf32_Sword&   calcValue ) const
{
    // Do regular job
    Elf32_Word symbol;
    ELFIO_Err nRet = GetEntry( index, offset, symbol, type, addend );
    
    // Find the symbol
    Elf32_Word    size;
    unsigned char bind;
    unsigned char symbolType;
    Elf32_Half    section;
    nRet = m_pSymTbl->GetSymbol( symbol, symbolName, symbolValue, 
                                 size, bind, symbolType, section );
    
    if ( ERR_ELFIO_NO_ERROR == nRet ) { // Was it successfull?
        switch ( type ) {
        case R_386_NONE:        // none
            calcValue = 0;
            break;
        case R_386_32:          // S + A
            calcValue = symbolValue + addend;
            break;
        case R_386_PC32:        // S + A - P
            calcValue = symbolValue + addend - offset;
            break;
        case R_386_GOT32:       // G + A - P
            calcValue = 0;
            break;
        case R_386_PLT32:       // L + A - P
            calcValue = 0;
            break;
        case R_386_COPY:        // none
            calcValue = 0;
            break;
        case R_386_GLOB_DAT:    // S
        case R_386_JMP_SLOT:    // S
            calcValue = symbolValue;
            break;
        case R_386_RELATIVE:    // B + A
            calcValue = addend;
            break;
        case R_386_GOTOFF:      // S + A - GOT
            calcValue = 0;
            break;
        case R_386_GOTPC:       // GOT + A - P 
            calcValue = 0;
            break;
        default:                // Not recognized symbol!
            calcValue = 0;
            nRet = ERR_ELFIO_SYMBOL_ERROR;
            break;
        }
    }
    
    return nRet;
}
