
#ifndef PeWriter_h
#define PeWriter_h

#include <vector>
#include <map>


template <int bitwidth>
class PeWriter : ExeWriter
{

	private:
		const uint32_t file_alignment=PAGE_SIZE;
		const uint8_t dos_header[0x80]=
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

		using coff_header_t = 
			struct coff_header
			{
				uint32_t magic;
				uint16_t machine;
				uint16_t number_of_sections;
				uint32_t time_date_stamp;
				uint32_t file_pointer_to_symboltable;
				uint32_t number_of_symbols;
				uint16_t sizeof_opt_header;
				uint16_t characteristics;
			}; 

		using standard_coff_header_t =
		        struct standard_coff_header
			{
				uint16_t magic;
				uint8_t major_linker_version;
				uint8_t minor_linker_version;
				uint32_t sizeof_code;
				uint32_t sizeof_initd_data;
				uint32_t sizeof_uninitd_data;
				uint32_t entry_point;
				uint32_t base_of_code;
			}; 

		using win_specific_fields_t = 
			struct win_specific_fields
			{
				uint64_t image_base;
				uint32_t section_alignment;
				uint32_t file_alignment;
				uint16_t major_os_version;
				uint16_t minor_os_version;
				uint16_t major_image_version;
				uint16_t minor_image_version;
				uint16_t major_subsystem_version;
				uint16_t minor_subsystem_version;
				uint32_t win32_version;
				uint32_t sizeof_image;
				uint32_t sizeof_headers;
				uint32_t checksum;
				uint16_t subsystem;
				uint16_t dll_characteristics;
				uint64_t sizeof_stack_reserve;
				uint64_t sizeof_stack_commit;
				uint64_t sizeof_heap_reserve;
				uint64_t sizeof_heap_commit;
				uint32_t loader_flags;
				uint32_t num_rva_and_sizes;
			};

		using image_data_directory_t =
			struct image_data_directory
			{
				uint32_t virtual_address;
				uint32_t size;
			};

		// the characteristics bitmap for a section header
		using section_characteristics_t = 
			enum section_characteristics
			{
				IMAGE_SCN_CNT_CODE               = 0x00000020, // The section contains executable code.
				IMAGE_SCN_CNT_INITIALIZED_DATA   = 0x00000040, // The section contains initialized data.
				IMAGE_SCN_CNT_UNINITIALIZED_DATA = 0x00000080, // The section contains uninitialized data.
				IMAGE_SCN_GPREL                  = 0x00008000, // The section contains data referenced through the global pointer (GP).
				IMAGE_SCN_LNK_NRELOC_OVFL        = 0x01000000, // The section contains extended relocations.
				IMAGE_SCN_MEM_DISCARDABLE        = 0x02000000, // The section can be discarded as needed.
				IMAGE_SCN_MEM_NOT_CACHED         = 0x04000000, // The section cannot be cached.
				IMAGE_SCN_MEM_NOT_PAGED          = 0x08000000, // The section is not pageable.
				IMAGE_SCN_MEM_SHARED             = 0x10000000, // The section can be shared in memory.
				IMAGE_SCN_MEM_EXECUTE            = 0x20000000, // The section can be executed as code.
				IMAGE_SCN_MEM_READ               = 0x40000000, // The section can be read.
				IMAGE_SCN_MEM_WRITE              = 0x80000000  // The section can be written to.
			};

		// Each entry in the section table is a section header.  In PE, a section is equiv. to an Elf Segment.
		// Section headers are fixed size, following this format:
		struct pe_section_header_t
		{
			pe_section_header_t(const LoadSegment_t* seg, FileIR_t* p_firp)
				:
					name{}
			{
				static int secno=0;
				/*
				 * uint64_t filesz;
				 * uint64_t memsz;
				 * uint64_t filepos;
				 * uint64_t start_page;
				 * uint64_t m_perms;
				 */
				snprintf(name, sizeof(name), "sec%d", secno++);
				virtual_size=seg->memsz;
				virtual_addr=seg->start_page-p_firp->getArchitecture()->getFileBase();
				file_size=seg->memsz;
				file_pointer_to_data=0; // fixed up later
				file_pointer_to_relocs=0;
				file_pointer_to_line_numbers=0;
				num_relocs=0;
				num_line_numbers=0;
				characteristics=0;

				const auto is_exe   = (seg->m_perms&0b1)==0b1;
				const auto is_write = (seg->m_perms&0b10)==0b10;
				const auto is_read  = (seg->m_perms&0b100)==0b100;

				// set code bits or data bits
				if(is_exe)
					characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE;
				else
					characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA;

				// set write bits
				if(is_write)
					characteristics |= IMAGE_SCN_MEM_WRITE;

				// set read bits
				if(is_read)
					characteristics |= IMAGE_SCN_MEM_READ;


				
			}

			char name[8]; // no null terminator, but null padded if <8 bytes
			uint32_t virtual_size; // size of the section in memory
			uint32_t virtual_addr; // virtual offset from image_base
			uint32_t file_size; // initialized data size, rounded up to file_alignment.
			uint32_t file_pointer_to_data; // must be multiple of file alignment from opt header.
			uint32_t file_pointer_to_relocs; // must be 0 for images.
			uint32_t file_pointer_to_line_numbers; // must be 0 for images.
			uint16_t num_relocs; // must be 0 for images.
			uint16_t num_line_numbers; // must be 0 for images.
			uint32_t characteristics;
		};


		coff_header_t                       coff_header_hdr={};
		standard_coff_header_t              standard_coff_header_hdr={};
		win_specific_fields_t               win_specific_fields_hdr={};
		std::vector<image_data_directory_t> image_data_dir_hdrs={}; 
		std::vector<pe_section_header_t>    section_headers={};
		std::FILE*                          fout=nullptr;
		std::FILE*                          fin=nullptr;

	public: 
		PeWriter(EXEIO::exeio *exeiop, IRDB_SDK::FileIR_t* firp, bool write_sections, bool bss_opts) 
			: 
			ExeWriter(exeiop,firp,write_sections,bss_opts)
		{
		}
		virtual ~PeWriter() {}
		void Write(const std::string &out_file, const std::string &infile);
		void InitHeaders();
		void GenerateDataDirectory();
		void CalculateHeaderSizes();
		void CreateSectionHeaders();
		void WriteFilePass1();


	protected:
};


using PeWriter64 = PeWriter<64>;



#endif

