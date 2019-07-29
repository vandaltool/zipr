
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


#define ALLOF(a) begin(a),end(a)


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




template<int width, class  uintMa_t>
void PeWriter<width,uintMa_t>::Write(const string &out_file, const string &infile)
{
	// confirm our understanding of these fields
	assert(sizeof(coff_header_t)==24);
	assert(sizeof(standard_coff_header_t)==24);
	// assert(sizeof(win_specific_fields_t)==88); not so true on pe32.
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

template<int width, class  uintMa_t>
void PeWriter<width,uintMa_t>::InitHeaders()
{
	const auto pebliss=reinterpret_cast<pe_bliss::pe_base*>(m_exeiop->get_pebliss());
	assert(pebliss);

	const auto orig_file_full_headers_str     = pebliss -> get_full_headers_data();
	const auto orig_file_full_headres_cstr    = orig_file_full_headers_str.c_str();
	const auto orig_file_standard_coff_header = (standard_coff_header_t*)(orig_file_full_headres_cstr+sizeof(dos_header)+sizeof(coff_header_t));

	// calculate the total size (last-first), rounded to page boundaries so that the OS can allocate that much virtual memory
	// for this object.
	const auto image_base = m_firp->getArchitecture()->getFileBase();
	const auto image_size = page_round_up(DetectMaxAddr()) - page_round_down(image_base);


	const auto machine = (width == 64) ? 0x8664 : // x86 64
	                     (width == 32) ? 0x014c : // i386
			     throw invalid_argument("Cannot map width to machine type");

	// initialize the headers
	coff_header_hdr = coff_header_t
		({
		 	0x00004550,                    // "PE\0\0"
			machine,                       // x86 64
			(uint16_t)segvec.size(),       // size of the segment map
			(uint32_t)time(nullptr),       // time in seconds
			0,                             // ?? have to figure file pointer to symtable out.
			0,                             // no symbols
			0,                             // size of optional headers -- calc'd below.
			pebliss->get_characteristics() // relocs stripped |  executable | line numbers stripped | large addresses OK
	       } );

	const auto magic_no = width==64 ?  0x20b :  // PE32+
	                      width==32 ?  0x10b :  // PE32
		              throw invalid_argument("Width -> magic cannot be calculated");

	standard_coff_header_hdr = standard_coff_header_t
		( {
		  	magic_no,
			orig_file_standard_coff_header->major_linker_version,  // version 2.25 linker (major part)
			orig_file_standard_coff_header->minor_linker_version,  // version 2.25 linker (minor part)
			0x1234,                                                // ?? have to figure out sizeof code
			0x5678,                                                // ?? have to figure out initd data
			0x4321,                                                // ?? have to figure out uninitd data
			(uint32_t)(m_exeiop->get_entry() - image_base),        // entry point
			0x1000                                                 // ?? have to figure out code base
		} );
	base_of_data=0x2000;


	win_specific_fields_hdr = win_specific_fields_t
		( {
		uintMa_t(image_base),                           // image_base;
		PAGE_SIZE,                                      // section_alignment;
		512,                                            // file_alignment -- guessing this is a magic constant we can always use
		pebliss->get_major_os_version(),                // major_os_version -- constants, may very if we need to port to more win versions.  read from a.ncexe?
		pebliss->get_minor_os_version(),                // minor_os_version;
		1,                                              // major_image_version;
		0,                                              // minor_image_version;
		pebliss->get_major_subsystem_version(),         // major_subsystem_version;
		pebliss->get_minor_subsystem_version(),         // minor_subsystem_version;
		0,                                              // win32_version;
		(uint32_t)image_size,                           // sizeof_image in memory (not including headers?)
		0x1000,                                         // sizeof_headers (OK to over estimate?)
		0,                                              // checksum ?? need to fix later
		3,                                              // subsystem ?? read from input file?
		pebliss->get_dll_characteristics(),             // dll_characteristics 
		uintMa_t(pebliss->get_stack_size_reserve_64()), // sizeof_stack_reserve
		uintMa_t(pebliss->get_stack_size_commit_64 ()), // sizeof_stack_commit
		uintMa_t(pebliss->get_heap_size_reserve_64 ()), // sizeof_heap_reserve
		uintMa_t(pebliss->get_heap_size_commit_64  ()), // sizeof_heap_commit
		0,                                              // loader_flags -- reserved, must be 0.
		0x10                                            // number of rva_and_sizes -- always 16 from what I can tell?
		} );

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


	auto exc_dir_it = find_if(ALLOF(m_firp->getDataScoops()), 
		[](const DataScoop_t* s)
		{
			return s->getName() == ".zipr_sehd";
		});
	if(exc_dir_it != end(m_firp->getDataScoops()))
	{
		const auto sehd_scoop=*exc_dir_it;
		image_data_dir_hdrs[pe_bliss::pe_win::image_directory_entry_exception].virtual_address = sehd_scoop->getStart()->getVirtualOffset() - m_firp->getArchitecture()->getFileBase();
		image_data_dir_hdrs[pe_bliss::pe_win::image_directory_entry_exception].size            = sehd_scoop->getSize();
	}

	CreateSectionHeaders();
}


template<int width, class  uintMa_t>
void  PeWriter<width,uintMa_t>::GenerateDataDirectory()
{
}

template<int width, class  uintMa_t>
void  PeWriter<width,uintMa_t>::CalculateHeaderSizes()
{
	// shorten name
	auto &hdr_size=coff_header_hdr.sizeof_opt_header;

	hdr_size  = 0;
	hdr_size += sizeof(standard_coff_header_t);
	if(width==32)
		hdr_size+=sizeof(uintMa_t); // add in BaseOfData field in sizing.
	hdr_size += sizeof(win_specific_fields_t);
	hdr_size += sizeof(image_data_directory_t) * image_data_dir_hdrs.size();

	// for each section header (but the last), look to make sure the section fills
	// the area until the start of the next section
	for(auto i=0u; i<section_headers.size()-1; i++)
	{
		// get header entry I and i+1.
		auto       &shi   = section_headers[i  ];
		const auto &ship1 = section_headers[i+1];

		// set the size to the only valid value.
		shi.virtual_size = ship1.virtual_addr - shi.virtual_addr;
	}

}

template<int width, class  uintMa_t>
void  PeWriter<width,uintMa_t>::CreateSectionHeaders()
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
	standard_coff_header_hdr.sizeof_code         = code_size;
	standard_coff_header_hdr.sizeof_initd_data   = init_data_size;
	standard_coff_header_hdr.sizeof_uninitd_data = uninit_data_size;

}


template<int width, class  uintMa_t>
void  PeWriter<width,uintMa_t>::WriteFilePass1()
{
	const auto res1=fseek(fout, 0,SEEK_SET);
	assert(res1==0);
	fwrite(&dos_header              , sizeof(dos_header)              , 1, fout);
	fwrite(&coff_header_hdr         , sizeof(coff_header_hdr)         , 1, fout);
	fwrite(&standard_coff_header_hdr, sizeof(standard_coff_header_hdr), 1, fout);
	if(width==32)
		fwrite(&base_of_data    , sizeof(base_of_data), 1, fout);
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
			const auto &page = pagemap[file_base+i];
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


}

template class PeWriter<64, uint64_t>;
template class PeWriter<32, uint32_t>;
// class PeWriter32; // instantiate templates.
// class PeWriter64;
