/*
ELFODynamic.cpp - ELF dynamic section producer.
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


ELFODynamicWriter::ELFODynamicWriter( IELFO* pIELFO, IELFOSection* pSection ) :
        m_nRefCnt( 1 ),
        m_pIELFO( pIELFO ),
        m_pSection( pSection )
{
    m_pIELFO->AddRef();
    m_pSection->AddRef();
}


ELFODynamicWriter::~ELFODynamicWriter()
{
}


int
ELFODynamicWriter::AddRef()
{
    m_pIELFO->AddRef();
    m_pSection->AddRef();
    return ++m_nRefCnt;
}


int
ELFODynamicWriter::Release()
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


ELFIO_Err
ELFODynamicWriter::AddEntry( Elf32_Sword tag, Elf32_Word value )
{
    Elf32_Dyn entry;
    entry.d_tag      = Convert32Sword2Host( tag, m_pIELFO->GetEncoding() );
    entry.d_un.d_val = Convert32Word2Host( value, m_pIELFO->GetEncoding() );

    return m_pSection->AddData( reinterpret_cast<const char*>( &entry ),
                                sizeof( entry ) );
}
