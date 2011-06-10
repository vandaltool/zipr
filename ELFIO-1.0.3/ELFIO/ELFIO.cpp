/*
ELFIO.cpp - ELFIO objects creator.
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


#include "ELFIO.h"
#include "ELFIImpl.h"
#include "ELFOImpl.h"


ELFIO::ELFIO()
{
}


ELFIO::ELFIO( const ELFIO& )
{
}


const ELFIO*
ELFIO::GetInstance()
{
    static ELFIO instance;

    return &instance;
}


ELFIO_Err
ELFIO::CreateELFI( IELFI** ppObj ) const
{
    ELFIO_Err bRet = ERR_ELFIO_NO_ERROR;
    *ppObj = new ELFI;
    if ( 0 == *ppObj ) {
      bRet = ERR_ELFIO_MEMORY;
    }

    return bRet;
}


ELFIO_Err
ELFIO::CreateELFO( IELFO** ppObj ) const
{
    ELFIO_Err bRet = ERR_ELFIO_NO_ERROR;
    *ppObj = new ELFO;
    if ( 0 == *ppObj ) {
      bRet = ERR_ELFIO_MEMORY;
    }

    return bRet;
}


std::string
ELFIO::GetErrorText( ELFIO_Err err ) const
{
    switch ( err ) {
    case ERR_ELFIO_NO_ERROR:         // No error
        return "No error";
        break;
    case ERR_ELFIO_INITIALIZED:      // The ELFIO object was initialized
        return "The ELFIO object initialized";
        break;
    case ERR_ELFIO_MEMORY:           // Out of memory
        return "Out of memory";
        break;
    case ERR_ELFIO_CANT_OPEN:        // Can't open a specified file
        return "Can't open a specified file";
        break;
    case ERR_ELFIO_NOT_ELF:          // The file is not a valid ELF file
        return "The file is not a valid ELF file";
        break;
    case ERR_NO_SUCH_READER:         // There is no such reader
        return "There is no such reader";
        break;
    case ERR_ELFIO_SYMBOL_ERROR:     // Symbol section reader error
        return "Symbol section reader error";
        break;
    case ERR_ELFIO_RELOCATION_ERROR: // Relocation section reader error
        return "Relocation section reader error";
        break;
    default:
        return "Unknown error code";
        break;
    }
}
