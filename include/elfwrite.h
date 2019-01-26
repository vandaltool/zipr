
#ifndef EhWriter_h
#define EhWriter_h

#include <vector>
#include <map>


#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

class ElfWriter
{
	protected: 

	class StringTable_t
	{
		public:

		StringTable_t() { } ;
		void AddString(const std::string &s)
		{
			if(locations.find(s)!=locations.end()) 
				return;
		
			locations[s]=table.size();
			table+=s;
			table+='\0';
		}
		void Write(FILE* fout) const
		{
			fwrite(table.c_str(), table.size(), 1, fout);
		}
		std::size_t size() const { return table.size(); }
		std::size_t location(const std::string &s) const { return locations.at(s); }
	
		private:

		std::string table;
		std::map<std::string,std::size_t> locations;
	};

	class PageData_t
	{
		
		public: 	
			PageData_t() : m_perms(0), is_relro(false), data(PAGE_SIZE), inuse(PAGE_SIZE) { }

			void union_permissions(int p_perms) { m_perms|=p_perms; }

			bool is_zero_initialized() const
			{ 
				for(unsigned int i=0;i<data.size();i++)
				{
					if(data.at(i)!=0)
						return false;
				}
				return true;
			}

		int m_perms;
		bool is_relro;

		std::vector<unsigned char> data;
		std::vector<bool> inuse;
	};
	class LoadSegment_t
	{
		public:
			LoadSegment_t() :filesz(0), memsz(0), filepos(0), start_page(0), m_perms(0) { }

			LoadSegment_t( unsigned int p_filesz, unsigned int p_memsz, unsigned int p_filepos, unsigned int p_start_page, unsigned int p_m_perms)
				:
				filesz(p_filesz),
				memsz(p_memsz), 
				filepos(p_filepos),
				start_page(p_start_page),
				m_perms(p_m_perms)
			{
				
			}


		unsigned int filesz; 
		unsigned int memsz; 
		unsigned int filepos;
		unsigned int start_page;
		unsigned int m_perms;
			
	};
	typedef std::vector<LoadSegment_t*> LoadSegmentVector_t;
	
	typedef std::map<IRDB_SDK::VirtualOffset_t, PageData_t> PageMap_t;

	public: 
		ElfWriter(IRDB_SDK::FileIR_t* firp, bool write_sections, bool bss_opts) : m_firp(firp), m_write_sections(write_sections), m_bss_opts(bss_opts) { }
		virtual ~ElfWriter() {}
		void Write(const ELFIO::elfio *elfiop, IRDB_SDK::FileIR_t* firp, const std::string &out_file, const std::string &infile);


	protected:

		virtual int getFileHeaderSize()=0;
		virtual int getSegHeaderSize()=0;
		virtual void LoadEhdr(FILE* fin)=0;
		virtual void LoadPhdrs(FILE* fin)=0;
		virtual void CreateNewPhdrs(const IRDB_SDK::VirtualOffset_t &min_addr, const IRDB_SDK::VirtualOffset_t &max_addr)=0;
		virtual void WriteElf(FILE* fout)=0;
		virtual void AddSections(FILE* fout)=0;
	

		PageMap_t pagemap;
		LoadSegmentVector_t segvec;

		template <class T> static T page_align(const T& in)
		{

			return in&~(PAGE_SIZE-1);
		}



	protected:
		IRDB_SDK::FileIR_t* m_firp;
		bool m_write_sections;
		bool m_bss_opts;
	private:
		IRDB_SDK::VirtualOffset_t DetectMinAddr(const ELFIO::elfio *elfiop, IRDB_SDK::FileIR_t* firp, const std::string &out_file);
		IRDB_SDK::VirtualOffset_t DetectMaxAddr(const ELFIO::elfio *elfiop, IRDB_SDK::FileIR_t* firp, const std::string &out_file);

		void CreatePagemap(const ELFIO::elfio *elfiop, IRDB_SDK::FileIR_t* firp, const std::string &out_file);
		void CreateSegmap(const ELFIO::elfio *elfiop, IRDB_SDK::FileIR_t* firp, const std::string &out_file);
		void SortSegmap();



		
};


// 


template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr, class T_Elf_Shdr, class T_Elf_Sym, class T_Elf_Rel, class T_Elf_Rela, class T_Elf_Dyn>
class ElfWriterImpl : public ElfWriter
{
	public:

		ElfWriterImpl(IRDB_SDK::FileIR_t* firp, bool write_sections, bool bss_opts ) : ElfWriter(firp, write_sections, bss_opts) { } 
	
	protected:
		int getFileHeaderSize()  { return sizeof(T_Elf_Ehdr); } 
		int getSegHeaderSize()  { return sizeof(T_Elf_Phdr); } 
		void LoadEhdr(FILE* fin);
		void LoadPhdrs(FILE* fin);
		void CreateNewPhdrs(const IRDB_SDK::VirtualOffset_t &min_addr, const IRDB_SDK::VirtualOffset_t &max_addr);
		bool CreateNewPhdrs_PostAllocate(const IRDB_SDK::VirtualOffset_t &min_addr, const IRDB_SDK::VirtualOffset_t &max_addr);
		bool CreateNewPhdrs_FirstPageAllocate(const IRDB_SDK::VirtualOffset_t &min_addr, const IRDB_SDK::VirtualOffset_t &max_addr) ;
		bool readonly_space_at(const IRDB_SDK::VirtualOffset_t addr, const unsigned int size);
		int locate_segment_index(const IRDB_SDK::VirtualOffset_t addr);
		unsigned int count_filesz_to_seg(unsigned int seg);
		bool CreateNewPhdrs_GapAllocate(const IRDB_SDK::VirtualOffset_t &min_addr, const IRDB_SDK::VirtualOffset_t &max_addr);
		bool CreateNewPhdrs_PreAllocate(const IRDB_SDK::VirtualOffset_t &min_addr, const IRDB_SDK::VirtualOffset_t &max_addr);
		bool CreateNewPhdrs_internal(
			const IRDB_SDK::VirtualOffset_t &min_addr, 
			const IRDB_SDK::VirtualOffset_t &max_addr,
			const int &first_seg_file_offset,
			const bool &add_pt_load_for_phdr,
			const size_t phdr_map_offset,
			IRDB_SDK::VirtualOffset_t new_phdr_addr
			);
		IRDB_SDK::DataScoop_t* find_scoop_by_name(const std::string& name, IRDB_SDK::FileIR_t* );
		void AddSections(FILE* fout);
		void update_phdr_for_scoop_sections(IRDB_SDK::FileIR_t* );
		void WriteElf(FILE* fout);
	private:
		unsigned int DetermineMaxPhdrSize();

		T_Elf_Ehdr ehdr;
		T_Elf_Ehdr new_ehdr;
		std::vector<T_Elf_Phdr> phdrs;
		std::vector<T_Elf_Phdr> new_phdrs;
};

typedef class ElfWriterImpl<ELFIO::Elf64_Ehdr, ELFIO::Elf64_Phdr, ELFIO::Elf64_Addr, 
	ELFIO::Elf64_Shdr, ELFIO::Elf64_Sym, ELFIO::Elf64_Rel, ELFIO::Elf64_Rela, ELFIO::Elf64_Dyn> ElfWriter64;

typedef class ElfWriterImpl<ELFIO::Elf32_Ehdr, ELFIO::Elf32_Phdr, ELFIO::Elf32_Addr, 
	ELFIO::Elf32_Shdr, ELFIO::Elf32_Sym, ELFIO::Elf32_Rel, ELFIO::Elf32_Rela, ELFIO::Elf32_Dyn> ElfWriter32;



#endif

