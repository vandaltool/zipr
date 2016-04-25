
#ifndef ElfWriter_h
#define ElfWriter_h

#include <vector>
#include <map>


#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

class ElfWriter
{
	protected: 

	class PageData_t
	{
		
		public: 	
			PageData_t() : m_perms(0), data(PAGE_SIZE) { }

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

		std::vector<unsigned char> data;
	};
	class LoadSegment_t
	{
		public:
			LoadSegment_t() :filesz(0), memsz(0), filepos(0), start_page(0) { }

		int filesz; 
		int memsz; 
		int filepos;
		int start_page;
		int m_perms;
			
	};
	typedef std::vector<LoadSegment_t*> LoadSegmentVector_t;
	
	typedef std::map<libIRDB::virtual_offset_t, PageData_t> PageMap_t;

	public: 
		virtual ~ElfWriter() {}
		void Write(const ELFIO::elfio *elfiop, libIRDB::FileIR_t* firp, const std::string &out_file, const std::string &infile);


	protected:

		virtual int GetFileHeaderSize()=0;
		virtual int GetSegHeaderSize()=0;
		virtual void LoadEhdr(FILE* fin)=0;
		virtual void LoadPhdrs(FILE* fin)=0;
		virtual void CreateNewEhdr()=0;
		virtual void CreateNewPhdrs()=0;
		virtual void WriteElf(FILE* fout)=0;
	

		PageMap_t pagemap;
		LoadSegmentVector_t segvec;

	private:
		libIRDB::virtual_offset_t DetectMinAddr(const ELFIO::elfio *elfiop, libIRDB::FileIR_t* firp, const std::string &out_file);
		void CreatePagemap(const ELFIO::elfio *elfiop, libIRDB::FileIR_t* firp, const std::string &out_file);
		void CreateSegmap(const ELFIO::elfio *elfiop, libIRDB::FileIR_t* firp, const std::string &out_file);
		
};

class ElfWriter64 : public ElfWriter
{
	public:

		ElfWriter64() { } 
	
	protected:
		int GetFileHeaderSize()  { return sizeof(ELFIO::Elf64_Ehdr); } 
		int GetSegHeaderSize()  { return sizeof(ELFIO::Elf64_Phdr); } 
		void LoadEhdr(FILE* fin) 
		{
			fseek(fin,0,SEEK_SET);
			fread(&ehdr,sizeof(ehdr), 1, fin);
		};
		void LoadPhdrs(FILE* fin) 
		{
			fseek(fin,ehdr.e_phoff,SEEK_SET);
			phdrs.resize(ehdr.e_phnum);
			for(unsigned int i=0;i<phdrs.size();i++)
			{
				fread(&phdrs[i], sizeof(phdrs[i]), 1, fin);
			}
		};
		void CreateNewEhdr() 
		{
			new_ehdr=ehdr;
			new_ehdr.e_shoff=0;
			new_ehdr.e_shnum=0;
			new_ehdr.e_phnum=new_phdrs.size();
			new_ehdr.e_phoff=sizeof(new_ehdr);
			new_ehdr.e_shstrndx=0;
		}
		void CreateNewPhdrs() 
		{

			// assume hdr and phdrs are on first page.
			unsigned int fileoff=0x1000;
			for(unsigned int i=0;i<segvec.size();i++)
			{
				ELFIO::Elf64_Phdr newphdr=
					{  /* p_type */ PT_LOAD, 
					   /* p_flags */(ELFIO::Elf_Word)segvec[i]->m_perms,      
					   /* p_offset */fileoff,     
					   /* p_vaddr */(ELFIO::Elf64_Addr)segvec[i]->start_page, 
					   /* p_paddr */(ELFIO::Elf64_Addr)segvec[i]->start_page, 
					   /* p_filesz */(ELFIO::Elf_Xword)segvec[i]->filesz, 
					   /* p_memsz */(ELFIO::Elf_Xword)segvec[i]->memsz, 
					   /* p_align */0x1000   };

				new_phdrs.push_back(newphdr);

				// advance file pointer.
				fileoff+=segvec[i]->filesz;
			}

			for(unsigned int i=0;i<phdrs.size();i++)
			{
				// skip any load sections, the irdb tells us what to load.
				if(phdrs[i].p_type == PT_LOAD)
					continue;

				if(phdrs[i].p_type == PT_PHDR)
					continue;

				ELFIO::Elf64_Phdr newphdr=phdrs[i];

				// find offset in loadable segment
				// using segvec.size() instead of new_phdrs size to search for segments in the new list.
				for(unsigned int j=0;j<segvec.size();j++)
				{
					if(new_phdrs[j].p_vaddr <= newphdr.p_vaddr  && newphdr.p_vaddr <= new_phdrs[j].p_vaddr+new_phdrs[j].p_filesz)
					{
						newphdr.p_offset=new_phdrs[j].p_offset+(newphdr.p_vaddr - new_phdrs[j].p_vaddr);
						break;
					}
				}
				// update offset
				new_phdrs.push_back(newphdr);
			}

			// specify a load section for the new program's header and phdr
			ELFIO::Elf64_Phdr newheaderphdr=
				{  /* p_type */ PT_LOAD,
				   /* p_flags */(ELFIO::Elf_Word)4,
				   /* p_offset */0,
				   /* p_vaddr */(ELFIO::Elf64_Addr)0x40000-PAGE_SIZE,
				   /* p_paddr */(ELFIO::Elf64_Addr)0x40000-PAGE_SIZE,
				   /* p_filesz */(ELFIO::Elf_Xword)PAGE_SIZE,
				   /* p_memsz */(ELFIO::Elf_Xword)PAGE_SIZE,
				   /* p_align */0x1000   };

			new_phdrs.insert(new_phdrs.begin(),newheaderphdr);
			ELFIO::Elf64_Phdr newphdrphdr=
				{  /* p_type */ PT_PHDR,
				   /* p_flags */(ELFIO::Elf_Word)4,
				   /* p_offset */64,
				   /* p_vaddr */(ELFIO::Elf64_Addr)0x40000-PAGE_SIZE+64,
				   /* p_paddr */(ELFIO::Elf64_Addr)0x40000-PAGE_SIZE+64,
				   /* p_filesz */(ELFIO::Elf_Xword)(new_phdrs.size()+1)*GetSegHeaderSize(),
				   /* p_memsz */(ELFIO::Elf_Xword)(new_phdrs.size()+1)*GetSegHeaderSize(),
				   /* p_align */0x1000   };
			new_phdrs.push_back(newphdrphdr);

		}
		void WriteElf(FILE* fout)
		{
			assert(fout);

			// write the header.
			fseek(fout,0,SEEK_SET);
			fwrite(&new_ehdr, sizeof(new_ehdr), 1, fout);

			// write the phdrs
			// for(unsigned int i=0;i<new_phdrs.size();i++)
			//	fwrite(&new_phdrs[i], sizeof(new_phdrs[i]), 1, fout);
			fwrite(new_phdrs.data(), sizeof(new_phdrs[0]), new_phdrs.size(), fout);


			// write each phdr's file.
			for(unsigned int i=0;i<new_phdrs.size();i++)
			{
				if(new_phdrs[i].p_type==PT_LOAD && new_phdrs[i].p_offset!=0)
				{
					fseek(fout,new_phdrs[i].p_offset,SEEK_SET);
					for(unsigned int j=0;j<new_phdrs[i].p_filesz;j+=PAGE_SIZE)
					{
						const PageData_t &page=pagemap[new_phdrs[i].p_vaddr+j];
						fwrite(page.data.data(), PAGE_SIZE, 1, fout);
				
					}
				}
			}
		}

	private:

		ELFIO::Elf64_Ehdr ehdr;
		ELFIO::Elf64_Ehdr new_ehdr;
		std::vector<ELFIO::Elf64_Phdr> phdrs;
		std::vector<ELFIO::Elf64_Phdr> new_phdrs;
};

class ElfWriter32 : public ElfWriter
{
	public:

		ElfWriter32() { } 
	
	protected:
		int GetFileHeaderSize()  { return sizeof(ELFIO::Elf32_Ehdr); } 
		int GetSegHeaderSize()  { return sizeof(ELFIO::Elf32_Phdr); } 
		void LoadEhdr(FILE* fin) {}
		void LoadPhdrs(FILE* fin) {}
		virtual void CreateNewEhdr() {}
		void CreateNewPhdrs() {}
		void WriteElf(FILE* fout) {}
};



#endif

