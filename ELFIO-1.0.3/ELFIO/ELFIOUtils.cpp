/*
ELFIOUtils.cpp - Utility functions.
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

#include "ELFIOUtils.h"


static unsigned char
GetHostEncoding()
{
    static const int tmp = 1;
    if ( 1 == *(char*)&tmp ) {
        return ELFDATA2LSB;
    }
    else {
        return ELFDATA2MSB;
    }
}


Elf32_Addr
Convert32Addr2Host( Elf32_Addr addr,  unsigned char encoding )
{
    if ( GetHostEncoding() == encoding ) {
        return addr;
    }
    else {
        char ch;
        ch = *(char*)&addr;
        *(char*)&addr = *( (char*)&addr + 3 );
        *( (char*)&addr + 3 ) = ch;
        ch = *( (char*)&addr + 1 );
        *( (char*)&addr + 1 ) = *( (char*)&addr + 2 );
        *( (char*)&addr + 2 ) = ch;

        return addr;
    }
}


Elf32_Half
Convert32Half2Host( Elf32_Half half,  unsigned char encoding )
{
    if ( GetHostEncoding() == encoding ) {
        return half;
    }
    else {
        char ch;
        ch = *(char*)&half;
        *(char*)&half = *( (char*)&half + 1 );
        *( (char*)&half + 1 ) = ch;

        return half;
    }
}


Elf32_Off
Convert32Off2Host  ( Elf32_Off  off,   unsigned char encoding )
{
    if ( GetHostEncoding() == encoding ) {
        return off;
    }
    else {
        char ch;
        ch = *(char*)&off;
        *(char*)&off = *( (char*)&off + 3 );
        *( (char*)&off + 3 ) = ch;
        ch = *( (char*)&off + 1 );
        *( (char*)&off + 1 ) = *( (char*)&off + 2 );
        *( (char*)&off + 2 ) = ch;

        return off;
    }
}


Elf32_Sword
Convert32Sword2Host( Elf32_Sword word, unsigned char encoding )
{
    if ( GetHostEncoding() == encoding ) {
        return word;
    }
    else {
        char ch;
        ch = *(char*)&word;
        *(char*)&word = *( (char*)&word + 3 );
        *( (char*)&word + 3 ) = ch;
        ch = *( (char*)&word + 1 );
        *( (char*)&word + 1 ) = *( (char*)&word + 2 );
        *( (char*)&word + 2 ) = ch;

        return word;
    }
}


Elf32_Word
Convert32Word2Host( Elf32_Word word,  unsigned char encoding )
{
    if ( GetHostEncoding() == encoding ) {
        return word;
    }
    else {
        char ch;
        ch = *(char*)&word;
        *(char*)&word = *( (char*)&word + 3 );
        *( (char*)&word + 3 ) = ch;
        ch = *( (char*)&word + 1 );
        *( (char*)&word + 1 ) = *( (char*)&word + 2 );
        *( (char*)&word + 2 ) = ch;

        return word;
    }
}


Elf32_Word
ElfHashFunc( const unsigned char* name )
{
    Elf32_Word h = 0, g;

    while ( *name ) {
        h = ( h << 4 ) + *name++;
        if ( g = h & 0xF0000000 ) {
            h ^= g >> 24;
        }
        h &= ~g;
    }
    
    return h;
}
