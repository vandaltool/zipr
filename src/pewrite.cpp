
#include <zipr_all.h>
#include <irdb-core>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <assert.h>
#include <sys/mman.h>
#include <ctype.h>
#include <iostream>   
#include <string>     
#include <fstream>
#include <elf.h>
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <pe_bliss.h>
#pragma GCC diagnostic pop



using namespace IRDB_SDK;
using namespace std;
using namespace zipr;
using namespace EXEIO;

uint8_t dos_header[]= 
	{
	/* 00000000 */  0x4d, 0x5a, 0x90, 0x00, 0x03, 0x00, 0x00, 0x00,  0x04, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,  // |MZ..............|
	/* 00000010 */  0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // |........@.......|
	/* 00000020 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // |................|
	/* 00000030 */  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,  // |................|
	/* 00000040 */  0x0e, 0x1f, 0xba, 0x0e, 0x00, 0xb4, 0x09, 0xcd,  0x21, 0xb8, 0x01, 0x4c, 0xcd, 0x21, 0x54, 0x68,  // |........!..L.!Th|
	/* 00000050 */  0x69, 0x73, 0x20, 0x70, 0x72, 0x6f, 0x67, 0x72,  0x61, 0x6d, 0x20, 0x63, 0x61, 0x6e, 0x6e, 0x6f,  // |is program canno|
	/* 00000060 */  0x74, 0x20, 0x62, 0x65, 0x20, 0x72, 0x75, 0x6e,  0x20, 0x69, 0x6e, 0x20, 0x44, 0x4f, 0x53, 0x20,  // |t be run in DOS |
	/* 00000070 */  0x6d, 0x6f, 0x64, 0x65, 0x2e, 0x0d, 0x0d, 0x0a,  0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   // |mode....$.......|
	};
/* 
 * notes:
 *      0) A PE32+ file starts with a dos header (explained above).  This is mostly ignored, except the word at byte 0x3c.
 *	1) The 4-bytes at location 0x3c specify where the PE file header starts, this is the real meat of the heading.
 *	2) the PE file header comes next.
 *	3) The PE file header starts with the COFF header
 *	3) The PE file header continues with the standard COFF header (which is diff than the COFF header)
 */




template<int width>
void PeWriter<width>::Write(const string &out_file, const string &infile)
{
	// confirm our understanding of these fields
	assert(sizeof(coff_header_t)==24);
	assert(sizeof(standard_coff_header_t)==24);
	assert(sizeof(win_specific_fields_t)==88);
	assert(sizeof(pe_section_header_t)==40);

	// open input/output files
	fin=fopen(infile.c_str(), "r");
        fout=fopen(out_file.c_str(), "w");
        assert(fin && fout);

	// create the maps of the memory layout.
        CreatePagemap();
        CreateSegmap();
	InitHeaders();
	GenerateDataDirectory();
	CalculateHeaderSizes();
	WriteFilePass1();
	WriteFilePass1();

	/* close output files */
	fclose(fin);
	fclose(fout);

	return;

}

template<int width>
void PeWriter<width>::InitHeaders()
{

	// initialize the headers
	coff_header_hdr = coff_header_t
		({
		 	0x00004550,    // "PE\0\0"
			0x8664,        // x86 64
			(uint16_t)segvec.size(), 
			               // size of the segment map
			(uint32_t)time(nullptr), 
			               // time in seconds
			0,             // ?? have to figure file pointer to symtable out.
			0,             // no symbols
			0,             // ?? have to figure size of optional headers out.
			0x2f           // relocs stripped |  executable | line numbers stripped | large addresses OK
	       } );


	standard_coff_header_hdr = standard_coff_header_t
		( {
			0x20b,  // PE32+
			2,      // version 2.25 linker (major part)
			25,     // version 2.25 linker (minor part)
			0x1234, // ?? have to figure out sizeof code
			0x5678, // ?? have to figure out initd data
			0x4321, // ?? have to figure out uninitd data
			0x1000, // ?? entry point
			0x1000  // ?? have to figure out code base
		} );


	win_specific_fields_hdr = win_specific_fields_t
		( {
		m_firp->getArchitecture()->getFileBase(), 
		           // image_base;
		PAGE_SIZE, // section_alignment;
		512,       // ?? file_alignment -- guessing this is a magic constant?
		4,         // major_os_version -- constants, may very if we need to port to more win versions.  read from a.ncexe?
		0,         // minor_os_version;
		0,         // major_image_version;
		0,         // minor_image_version;
		5,         // major_subsystem_version;
		2,         // minor_subsystem_version;
		0,         // win32_version;
		0,         // ?? sizeof_image in memory.  need to figure out.  we're creating constant headers, right?  maybe?
		0x400,     // sizeof_headers;
		0,         // checksum ?? need to fix later
		3,         // subsystem ?? read from input file?
		0x8000,    // dll_characteristics ?? read from input file?
		0x200000,  // sizeof_stack_reserve -- maybe read from input file?
		0x1000,    // sizeof_stack_commit -- maybe read from input file?
		0x100000,  // sizeof_heap_reserve -- maybe read from input file?
		0x1000,    // sizeof_heap_commit -- maybe read from input file?
		0,         // loader_flags -- reserved, must be 0.
		0x10       // number of rva_and_sizes -- always 16 from what I can tell?

		} );

	const auto pebliss=reinterpret_cast<pe_bliss::pe_base*>(m_exeiop->get_pebliss());
	assert(pebliss);

	//
	// Get data directories from pebliss
	//
	// magic number 16:  the number of data directories in a PE32+ file for windows?  not clear why this is the value or how to get it from pebliss.
	for(auto id = 0u; id < 16; id++) 
	{
		const auto rva      = pebliss->get_directory_rva(id);
		const auto rva_size = pebliss->get_directory_size(id);
		cout<<"rva/size pair is "<<hex << rva << "/" << rva_size << endl;
		image_data_dir_hdrs.push_back({rva, rva_size});
	}


	CreateSectionHeaders();
}


template<int width>
void  PeWriter<width>::GenerateDataDirectory()
{
}

template<int width>
void  PeWriter<width>::CalculateHeaderSizes()
{
	// shorten name
	auto &hdr_size=coff_header_hdr.sizeof_opt_header;

	hdr_size  = 0;
	hdr_size += sizeof(standard_coff_header_t);
	hdr_size += sizeof(win_specific_fields_t);
	hdr_size += sizeof(image_data_directory_t) * image_data_dir_hdrs.size();

}

template<int width>
void  PeWriter<width>::CreateSectionHeaders()
{
	auto code_size=0u;
	auto init_data_size=0u;
	auto uninit_data_size=0u;

	auto sh=vector<pe_section_header_t>();
	for(auto seg : segvec)
	{
		section_headers.push_back({seg,m_firp});
		if((seg->m_perms & 0b1 ) == 0b1)
			code_size+=seg->memsz;
		else if((seg->m_perms & 0b100 ) == 0b100)
			init_data_size+=seg->memsz;
	}

	// update header with section size info.
	standard_coff_header_hdr.sizeof_code          = code_size;
	standard_coff_header_hdr.sizeof_initd_data   = init_data_size;
	standard_coff_header_hdr.sizeof_uninitd_data = uninit_data_size;

}


template<int width>
void  PeWriter<width>::WriteFilePass1()
{
	const auto res1=fseek(fout, 0,SEEK_SET);
	assert(res1==0);
	fwrite(&dos_header              , sizeof(dos_header)              , 1, fout);
	fwrite(&coff_header_hdr         , sizeof(coff_header_hdr)         , 1, fout);
	fwrite(&standard_coff_header_hdr, sizeof(standard_coff_header_hdr), 1, fout);
	fwrite(&win_specific_fields_hdr , sizeof(win_specific_fields_hdr) , 1, fout);

	fwrite(image_data_dir_hdrs.data(), sizeof(image_data_directory_t), image_data_dir_hdrs.size(), fout);
	fwrite(section_headers.data()    , sizeof(pe_section_header_t)   , section_headers.size()    , fout);

	auto file_base=m_firp->getArchitecture()->getFileBase();

	for(auto& s : section_headers)
	{
		// get the current file position, and round it up to the nearest file-alignment position.
		const auto pos=round_up_to(ftell(fout),file_alignment);

		// record it for pass2
		s.file_pointer_to_data=pos;

		// and jump there.
		const auto res2=fseek(fout, pos,SEEK_SET);
		assert(res2==0);

		// now write the info from the pages
		const auto seg_end_addr = s.virtual_addr + s.file_size;
		for(auto i = page_round_down(s.virtual_addr); i < seg_end_addr; i+=PAGE_SIZE)
		{
			const auto & page     = pagemap[file_base+i];
			for(auto j=0u; j<PAGE_SIZE; j++)
			{
				// skip bytes that aren't in the section.
				if(i+j < s.virtual_addr) continue; 
				if(i+j >= seg_end_addr ) continue; 

				const auto byte=page.data.at(j);
				fwrite(&byte, 1, 1, fout);
			}
		}
	}

	// move it out to file_alignment bytes.
	const auto end_pos=round_up_to(ftell(fout),file_alignment);
	const auto res3=fseek(fout, end_pos-1,SEEK_SET);
	assert(res3==0);
	const auto zero=uint8_t(0);
	fwrite(&zero,0,0,fout);	 // fill out the file with the last byte.

	// update the header with the total file size.
	win_specific_fields_hdr.sizeof_image=end_pos;



}

template class PeWriter<64>;
