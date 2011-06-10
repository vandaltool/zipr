/*
ELFDump.cpp - Dump ELF file using ELFIO library.
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

#include <stdio.h>
#include <string>
#include <ELFIO.h>

using namespace std;

void
PrintHeader( const IELFI* pReader )
{
    printf( "ELF Header\n" );
    printf( "  Class:      %s (%d)\n",
            ( ELFCLASS32 == pReader->GetClass() ) ? "CLASS32" : "Unknown",
            (int)pReader->GetClass() );
    if ( ELFDATA2LSB == pReader->GetEncoding() ) {
        printf( "  Encoding:   Little endian\n" );
    }
    else if ( ELFDATA2MSB == pReader->GetEncoding() ) {
        printf( "  Encoding:   Big endian\n" );
    }
    else {
        printf( "  Encoding:   Unknown\n" );
    }
    printf( "  ELFVersion: %s (%d)\n",
            ( EV_CURRENT == pReader->GetELFVersion() ) ? "Current" : "Unknown",
            (int)pReader->GetELFVersion() );
    printf( "  Type:       0x%04X\n",   pReader->GetType() );
    printf( "  Machine:    0x%04X\n",   pReader->GetMachine() );
    printf( "  Version:    0x%08X\n",   pReader->GetVersion() );
    printf( "  Entry:      0x%08X\n",   pReader->GetEntry() );
    printf( "  Flags:      0x%08X\n\n", pReader->GetFlags() );
}


string
SectionTypes( Elf32_Word type )
{
    string sRet = "UNKNOWN";
    switch ( type ) {
    case SHT_NULL :
        sRet = "NULL";
        break;
    case SHT_PROGBITS :
        sRet = "PROGBITS";
        break;
    case SHT_SYMTAB :
        sRet = "SYMTAB";
        break;
    case SHT_STRTAB :
        sRet = "STRTAB";
        break;
    case SHT_RELA :
        sRet = "RELA";
        break;
    case SHT_HASH :
        sRet = "HASH";
        break;
    case SHT_DYNAMIC :
        sRet = "DYNAMIC";
        break;
    case SHT_NOTE :
        sRet = "NOTE";
        break;
    case SHT_NOBITS :
        sRet = "NOBITS";
        break;
    case SHT_REL :
        sRet = "REL";
        break;
    case SHT_SHLIB :
        sRet = "SHLIB";
        break;
    case SHT_DYNSYM :
        sRet = "DYNSYM";
        break;
    }
    
    return sRet;
}


string SectionFlags( Elf32_Word flags )
{
    string sRet = "";
    if ( flags & SHF_WRITE ) {
        sRet += "W";
    }
    if ( flags & SHF_ALLOC ) {
        sRet += "A";
    }
    if ( flags & SHF_EXECINSTR ) {
        sRet += "X";
    }

    return sRet;
}


void
PrintSection( int i, const IELFISection* pSec )
{
    printf( "  [%2x] %-20s %-8.8s %08x %06x %02x %-3.3s %02x %04x %02x\n",
         i,
         string( pSec->GetName() ).substr( 0, 20 ).c_str(),
         SectionTypes( pSec->GetType() ).c_str(),
         pSec->GetAddress(),
         pSec->GetSize(),
         pSec->GetEntrySize(),
         SectionFlags( pSec->GetFlags() ).c_str(),
         pSec->GetLink(),
         pSec->GetInfo(),
         pSec->GetAddrAlign() );

    return;
}


string
SegmentTypes( Elf32_Word type )
{
    string sRet = "UNKNOWN";
    switch ( type ) {
    case PT_NULL:
        sRet = "NULL";
        break;
    case PT_LOAD:
        sRet = "PT_LOAD";
        break;
    case PT_DYNAMIC:        
        sRet = "PT_DYNAMIC";
        break;
    case PT_INTERP:
        sRet = "PT_INTERP";
        break;
    case PT_NOTE:
        sRet = "PT_NOTE";
        break;
    case PT_SHLIB:
        sRet = "PT_SHLIB";
        break;
    case PT_PHDR:
        sRet = "PT_PHDR";
        break;
    }
    
    return sRet;
}


void
PrintSegment( int i, const IELFISegment* pSeg )
{
    printf( "  [%2x] %-10.10s %08x %08x %08x %08x %08x %08x\n",
            i,
            SegmentTypes( pSeg->GetType() ).c_str(),
            pSeg->GetVirtualAddress(),
            pSeg->GetPhysicalAddress(),
            pSeg->GetFileSize(),
            pSeg->GetMemSize(),
            pSeg->GetFlags(),
            pSeg->GetAlign() );

    return;
}


void
PrintSymbol( std::string& name, Elf32_Addr value,
             Elf32_Word size,
             unsigned char bind, unsigned char type,
             Elf32_Half section )
{
    printf( "%-46.46s %08x %08x %04x %04x %04x\n",
            name.c_str(),
            value,
            size,
            (int)bind,
            (int)type,
            section );
}


int main( int argc, char** argv )
{
    if ( argc != 2 ) {
        printf( "Usage: ELFDump <file_name>\n" );
        return 1;
    }


    // Open ELF reader
    IELFI* pReader;
    if ( ERR_ELFIO_NO_ERROR != ELFIO::GetInstance()->CreateELFI( &pReader ) ) {
        printf( "Can't create ELF reader\n" );
        return 2;
    }

    if ( ERR_ELFIO_NO_ERROR != pReader->Load( argv[1] ) ) {
        printf( "Can't open input file \"%s\"\n", argv[1] );
        return 3;
    }

    // Print ELF file header
    PrintHeader( pReader );

    // Print ELF file sections
    printf( "Section headers:\n" );
    printf( "  [Nr] Name                 Type     Addr     Size   ES Flg Lk Inf  Al\n" );
    int nSecNo = pReader->GetSectionsNum();
    int i;
    for ( i = 0; i < nSecNo; ++i ) {    // For all sections
        const IELFISection* pSec = pReader->GetSection( i );
        PrintSection( i, pSec );
        pSec->Release();
    }
    printf( "Key to Flags: W (write), A (alloc), X (execute)\n\n" );

    // Print ELF file segments
    int nSegNo = pReader->GetSegmentsNum();
    if ( nSegNo > 0 ) {
        printf( "Segment headers:\n" );
        printf( "  [Nr] Type       VirtAddr PhysAddr FileSize Mem.Size Flags    Align\n" );
    }
    for ( i = 0; i < nSegNo; ++i ) {    // For all sections
        const IELFISegment* pSeg = pReader->GetSegment( i );
        PrintSegment( i, pSeg );
        pSeg->Release();
    }
    printf( "\n" );

    // Print symbol tables
    nSecNo = pReader->GetSectionsNum();
    for ( i = 0; i < nSecNo; ++i ) {    // For all sections
        const IELFISection* pSec = pReader->GetSection( i );
        if ( SHT_SYMTAB == pSec->GetType() || SHT_DYNSYM == pSec->GetType() ) {
            IELFISymbolTable* pSymTbl = 0;
            pReader->CreateSectionReader( IELFI::ELFI_SYMBOL, pSec, (void**)&pSymTbl );

            std::string   name;
            Elf32_Addr    value;
            Elf32_Word    size;
            unsigned char bind;
            unsigned char type;
            Elf32_Half    section;
            int nSymNo = pSymTbl->GetSymbolNum();
            if ( 0 < nSymNo ) {
                printf( "Symbol table (%s)\n", pSymTbl->GetName().c_str() );
                printf( "     Name                                      Value    Size     Bind Type Sect\n" );
                for ( int i = 0; i < nSymNo; ++i ) {
                    pSymTbl->GetSymbol( i, name, value, size, bind, type, section );
                    PrintSymbol( name, value, size, bind, type, section );
                }
            }

            pSymTbl->Release();
            printf( "\n" );
        }
        pSec->Release();
    }

    for ( i = 0; i < nSecNo; ++i ) {    // For all sections
        const IELFISection* pSec = pReader->GetSection( i );
        if ( SHT_NOTE == pSec->GetType() ) {
            IELFINoteReader* pNote;
            pReader->CreateSectionReader( IELFI::ELFI_NOTE, pSec, (void**)&pNote );
            int nNotesNo = pNote->GetNotesNum();
            if ( 0 < nNotesNo ) {
                printf( "Note section (%s)\n", pSec->GetName().c_str() );
                printf( "   No     Type    Name\n" );
                for ( int i = 0; i < nNotesNo; ++i ) {    // For all notes
                    Elf32_Word  type;
                	std::string name;
                	void* desc;
                    Elf32_Word descsz;
                
                    pNote->GetNote( i, type, name, desc, descsz );
                    printf( "  [%2d] 0x%08x %s\n", i, type, name.c_str() );
                }
            }
            
            pNote->Release();
            printf( "\n" );
        }
        pSec->Release();
    }

    pReader->Release();

    return 0;
}
