/*
ELFOString.cpp - ELF string section producer functionality.
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


ELFOStringWriter::ELFOStringWriter( IELFO* pIELFO, IELFOSection* pSection ) :
        m_nRefCnt( 1 ),
        m_pIELFO( pIELFO ),
        m_pSection( pSection )
{
    if ( 0 != pSection->GetData() && 0 != pSection->GetSize() ) {
        m_data.append( pSection->GetData(), pSection->GetSize() );
    }
    m_pIELFO->AddRef();
    m_pSection->AddRef();
}


ELFOStringWriter::~ELFOStringWriter()
{
}


int
ELFOStringWriter::AddRef()
{
    m_pIELFO->AddRef();
    m_pSection->AddRef();
    return ++m_nRefCnt;
}


int
ELFOStringWriter::Release()
{
    int nRet             = --m_nRefCnt;
    IELFO*        pIELFO = m_pIELFO;
    IELFOSection* pSec   = m_pSection;

    if ( 0 == m_nRefCnt ) {
        // Flush data
        m_pSection->SetData( m_data.data(), m_data.size() );
        
        delete this;
    }
    pSec->Release();
    pIELFO->Release();

    return nRet;
}


const char*
ELFOStringWriter::GetString( Elf32_Word index ) const
{
    if ( index < m_data.size() ) {
        const char* pData = m_data.data();
        if ( 0 != pData ) {
            return pData + index;
        }
    }
    
    return 0;
}


Elf32_Word
ELFOStringWriter::AddString( const char* str )
{
    Elf32_Word nRet = m_data.size();    

    if ( m_data.size() == 0 ) {
        m_data += '\0';
        nRet = 1;
    }
    m_data.append( str );
    m_data += '\0';
    
    return nRet;
}
