#include <cstdio>
#include <ELFIO.h>

int main( int, char* argv[] )
{
    // Create a ELFI reader
    IELFI* pReader;
    ELFIO::GetInstance()->CreateELFI( &pReader );

    // Initialize it
    char* filename = argv[1];
    pReader->Load( filename );

    // Get .text relocation entry
    // List all sections of the file
    int i;
    int nSecNo = pReader->GetSectionsNum();
    for ( i = 0; i < nSecNo; ++i ) {    // For all sections
        const IELFISection* pSec = pReader->GetSection( i );
        if ( SHT_REL != pSec->GetType() && SHT_RELA != pSec->GetType() ) {
            pSec->Release();
            continue;
        }
        const IELFIRelocationTable* pRel = 0;
        pReader->CreateSectionReader( IELFI::ELFI_RELOCATION, pSec, (void**)&pRel );

        // Print all entries
        Elf32_Addr     offset;
        Elf32_Addr     symbolValue;
        std::string    symbolName;
        unsigned char  type;
        Elf32_Sword    addend;
        Elf32_Sword    calcValue;
        Elf32_Word nNum = pRel->GetEntriesNum();
        if ( 0 < nNum ) {
            std::printf( "\nSection name: %s\n", pSec->GetName().c_str() );
            std::printf( "  Num Type Offset   Addend    Calc   SymValue   SymName\n" );
            for ( Elf32_Word i = 0; i < nNum; ++i ) {
                pRel->GetEntry( i, offset, symbolValue, symbolName,
                                type, addend, calcValue );
                std::printf( "[%4x] %02x %08x %08x %08x %08x %s\n",
                             i, type, offset,
                             addend, calcValue,
                             symbolValue, symbolName.c_str() );
            }
        }

        pSec->Release();
        pRel->Release();
    }

    // Free resources
    pReader->Release();

    return 0;
}
