
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
		virtual void CreateNewPhdrs(libIRDB::virtual_offset_t min_addr)=0;
		virtual void WriteElf(FILE* fout)=0;
	

		PageMap_t pagemap;
		LoadSegmentVector_t segvec;

		template <class T> static T page_align(T& in)
		{

			return in&~(PAGE_SIZE-1);
		}



	private:
		libIRDB::virtual_offset_t DetectMinAddr(const ELFIO::elfio *elfiop, libIRDB::FileIR_t* firp, const std::string &out_file);
		void CreatePagemap(const ELFIO::elfio *elfiop, libIRDB::FileIR_t* firp, const std::string &out_file);
		void CreateSegmap(const ELFIO::elfio *elfiop, libIRDB::FileIR_t* firp, const std::string &out_file);
		void SortSegmap();

		
};


// 


template <class T_Elf_Ehdr, class T_Elf_Phdr, class T_Elf_Addr>
class ElfWriterImpl : public ElfWriter
{
	public:

		ElfWriterImpl() { } 
	
	protected:
		int GetFileHeaderSize()  { return sizeof(T_Elf_Ehdr); } 
		int GetSegHeaderSize()  { return sizeof(T_Elf_Phdr); } 
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
		void CreateNewPhdrs(libIRDB::virtual_offset_t min_addr) 
		{

			// create a load segments into the new header list.
			// assume hdr and phdrs are on first page.
			unsigned int fileoff=0x1000;
			for(unsigned int i=0;i<segvec.size();i++)
			{
				T_Elf_Phdr newphdr;
				memset(&newphdr,0,sizeof(newphdr));
			   	newphdr.p_type = PT_LOAD; 
			   	newphdr.p_flags = (ELFIO::Elf_Word)segvec[i]->m_perms;      
			   	newphdr.p_offset = fileoff;     
			   	newphdr.p_vaddr = (T_Elf_Addr)segvec[i]->start_page; 
			   	newphdr.p_paddr = (T_Elf_Addr)segvec[i]->start_page; 
			   	newphdr.p_filesz = (ELFIO::Elf_Xword)segvec[i]->filesz; 
			   	newphdr.p_memsz = (ELFIO::Elf_Xword)segvec[i]->memsz; 
			   	newphdr.p_align=  0x1000;

				new_phdrs.push_back(newphdr);

				// advance file pointer.
				fileoff+=segvec[i]->filesz;
			}


			// go through orig. phdrs any copy any that aren't of type pt_load or pt_hdr
			for(unsigned int i=0;i<phdrs.size();i++)
			{
				// skip any load sections, the irdb tells us what to load.
				if(phdrs[i].p_type == PT_LOAD)
					continue;

				// skip phdr section.
				if(phdrs[i].p_type == PT_PHDR)
					continue;

				T_Elf_Phdr newphdr=phdrs[i];

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
			T_Elf_Phdr newheaderphdr;
			memset(&newheaderphdr,0,sizeof(newheaderphdr));
			newheaderphdr.p_type = PT_LOAD;
			newheaderphdr.p_flags =(ELFIO::Elf_Word)4;
			newheaderphdr.p_offset =0;
			newheaderphdr.p_vaddr =(T_Elf_Addr)page_align(min_addr)-PAGE_SIZE;
			newheaderphdr.p_paddr =(T_Elf_Addr)page_align(min_addr)-PAGE_SIZE;
			newheaderphdr.p_filesz =(ELFIO::Elf_Xword)PAGE_SIZE;
			newheaderphdr.p_memsz =(ELFIO::Elf_Xword)PAGE_SIZE;
			newheaderphdr.p_align =0x1000;

			// create the new phdr
			new_phdrs.insert(new_phdrs.begin(),newheaderphdr);
			T_Elf_Phdr newphdrphdr;
			memset(&newphdrphdr,0,sizeof(newphdrphdr));
			newphdrphdr.p_type = PT_PHDR;
			newphdrphdr.p_flags =(ELFIO::Elf_Word)4;
			newphdrphdr.p_offset =64;
			newphdrphdr.p_vaddr =(T_Elf_Addr)page_align(min_addr)-PAGE_SIZE+sizeof(T_Elf_Ehdr);
			newphdrphdr.p_paddr =(T_Elf_Addr)page_align(min_addr)-PAGE_SIZE+sizeof(T_Elf_Ehdr);
			newphdrphdr.p_filesz =(ELFIO::Elf_Xword)(new_phdrs.size()+1)*GetSegHeaderSize();
			newphdrphdr.p_memsz =(ELFIO::Elf_Xword)(new_phdrs.size()+1)*GetSegHeaderSize();
			newphdrphdr.p_align =0x1000;
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

						// is it the last page?
						if(j+PAGE_SIZE < new_phdrs[i].p_filesz)
						{
							fwrite(page.data.data(), PAGE_SIZE, 1, fout);
						}
						else
						{
							// don't write 0's at end of last page 
							int k=0;
							for (k=PAGE_SIZE;k>0;k--)
							{
								if(page.data[k]!=0)
									break;
							}
							std::cout<<"optimizing last page write to size k="<<std::hex<<k<<std::endl;
							fwrite(page.data.data(), k+1, 1, fout);
						}
				
					}
				}
			}
		}

	private:

		T_Elf_Ehdr ehdr;
		T_Elf_Ehdr new_ehdr;
		std::vector<T_Elf_Phdr> phdrs;
		std::vector<T_Elf_Phdr> new_phdrs;
};

typedef class ElfWriterImpl<ELFIO::Elf64_Ehdr, ELFIO::Elf64_Phdr, ELFIO::Elf64_Addr> ElfWriter64;
typedef class ElfWriterImpl<ELFIO::Elf32_Ehdr, ELFIO::Elf32_Phdr, ELFIO::Elf32_Addr> ElfWriter32;



#endif

