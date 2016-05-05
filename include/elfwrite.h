
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
			PageData_t() : m_perms(0), data(PAGE_SIZE), inuse(PAGE_SIZE) { }

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
		std::vector<bool> inuse;
	};
	class LoadSegment_t
	{
		public:
			LoadSegment_t() :filesz(0), memsz(0), filepos(0), start_page(0) { }

		unsigned int filesz; 
		unsigned int memsz; 
		unsigned int filepos;
		unsigned int start_page;
		unsigned int m_perms;
			
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
		virtual void CreateNewPhdrs(const libIRDB::virtual_offset_t &min_addr, const libIRDB::virtual_offset_t &max_addr)=0;
		virtual void WriteElf(FILE* fout)=0;
	

		PageMap_t pagemap;
		LoadSegmentVector_t segvec;

		template <class T> static T page_align(const T& in)
		{

			return in&~(PAGE_SIZE-1);
		}



	private:
		libIRDB::virtual_offset_t DetectMinAddr(const ELFIO::elfio *elfiop, libIRDB::FileIR_t* firp, const std::string &out_file);
		libIRDB::virtual_offset_t DetectMaxAddr(const ELFIO::elfio *elfiop, libIRDB::FileIR_t* firp, const std::string &out_file);

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
		void CreateNewPhdrs(const libIRDB::virtual_offset_t &min_addr, const libIRDB::virtual_offset_t &max_addr) 
		{
			
			if(CreateNewPhdrs_GapAllocate(min_addr, max_addr))
				return;
			else if(CreateNewPhdrs_FirstPageAllocate(min_addr, max_addr))
				return;
			else if(CreateNewPhdrs_PreAllocate(min_addr, max_addr))
				return;
			else if(CreateNewPhdrs_PostAllocate(min_addr, max_addr))
				return;
			else
				assert(0);
			
		}
		bool CreateNewPhdrs_PostAllocate(const libIRDB::virtual_offset_t &min_addr, const libIRDB::virtual_offset_t &max_addr) 
		{
			// post allocation not enabled, yet.
			return false;
		}
		bool CreateNewPhdrs_FirstPageAllocate(const libIRDB::virtual_offset_t &min_addr, const libIRDB::virtual_offset_t &max_addr) 
		{
			// check to see if there's room on the first page for 
			unsigned int phdr_size=DetermineMaxPhdrSize();
			if(page_align(min_addr)+sizeof(T_Elf_Ehdr)+phdr_size > min_addr)
				return false;
			// this is an uncommon case -- we are typically adding
			// segments and so the segment map won't fit on the first page.
			// if this assertion hits, email hiser@virginia.edu and attack input pgm,
			// then convert this to a return false to avoid assertion until he fixes it;
			assert(0);
		}

		bool readonly_space_at(const libIRDB::virtual_offset_t addr, const unsigned int size)
		{
			for(unsigned int i=0;i<size;i++)
			{
				libIRDB::virtual_offset_t page=page_align(addr+i);
				libIRDB::virtual_offset_t page_offset=addr+i-page;

				if(pagemap.at(page).inuse[page_offset])
					return false;

				// check write perms.  4=r, 2=w, 1=x.
				if((pagemap.at(page).m_perms & 0x2) != 0)
					return false;
			}
			return true;
		}
		int locate_segment_index(const libIRDB::virtual_offset_t addr)
		{
			// segment's are sorted by address.
			for(unsigned int i=0;i<segvec.size();i++)
			{
				// if the filesz diverges from the memsz, then we have an issue.
				// luckily, this only happens for bss, and there's almost always space
				// after the text for the new phdr.
				if(segvec[i]->filesz < segvec[i]->memsz)
					return -1; 

				// if the new_phdr_addr is in this segment.
				if(segvec[i]->start_page <= addr  && addr < segvec[i]->start_page + segvec[i]->filesz)
				{
					return i;
				}
			}
			return -1;
		}
		unsigned int count_filesz_to_seg(unsigned int seg)
		{
			unsigned int filesz=0;
			// segment's are sorted by address.
			for(unsigned int i=0;i<seg;i++)
			{
				filesz+=segvec[i]->filesz;
			}
			return filesz;
		}
		bool CreateNewPhdrs_GapAllocate(const libIRDB::virtual_offset_t &min_addr, const libIRDB::virtual_offset_t &max_addr) 
		{
			/* for shared objects, we need the PHDR file offset to be equal to the  
			 * memory offset because the kernel passes base_address+Ehdr::ph_off via
			 * the auxv array to ld.so.  Where ld.so then uses that as an address.
			 */


			// first, find the first free space that's big enough.
			unsigned int phdr_size=DetermineMaxPhdrSize();
			libIRDB::virtual_offset_t new_phdr_addr=0;
			for(unsigned int i=min_addr;i<max_addr; i++)
			{
				if(readonly_space_at(i,phdr_size))
				{
					new_phdr_addr=i;
					break;
				}
			
			}

			// find segment
			int new_phdr_segment_index=locate_segment_index(new_phdr_addr);
			if(new_phdr_segment_index==-1)
				return false;


			assert(phdr_size<PAGE_SIZE);
			// verify that the segment jjjjjjjjjjjjjjjj
			int pages_to_extend=false;	// extend the segment by a page.
			for(unsigned int i=0;i<phdr_size; i++)
			{
				// find segment for phdr+i
				int seg=locate_segment_index(new_phdr_addr+i);

				// if it's not allocated, we ran off the end of the segment.  
				// if we're also at the first byte of a page, extend the segment by a page.
				if(seg==-1 && page_align(new_phdr_addr+i)==new_phdr_addr+i)
					pages_to_extend++;

				// this should be safe because the new page can't be in a segment already.
				if(seg==-1)
					continue;

				if(seg == new_phdr_segment_index)
					continue;
				// uh oh, we found that the phdr would fit cross segment boundaries.	
				return false;
			}
			segvec[new_phdr_segment_index]->filesz+=(PAGE_SIZE*pages_to_extend);

			unsigned int fileoff=count_filesz_to_seg(new_phdr_segment_index);
			fileoff+=(new_phdr_addr-segvec[new_phdr_segment_index]->start_page);

			return CreateNewPhdrs_internal(min_addr,max_addr,0x0,false,fileoff, new_phdr_addr);

		}
	
		bool CreateNewPhdrs_PreAllocate(const libIRDB::virtual_offset_t &min_addr, const libIRDB::virtual_offset_t &max_addr) 
		{
#if 0
			if(page_align(min_addr)==0)
			//warning?
#endif

			libIRDB::virtual_offset_t new_phdr_addr=(T_Elf_Addr)page_align(min_addr)-PAGE_SIZE+sizeof(T_Elf_Ehdr);
			return CreateNewPhdrs_internal(min_addr,max_addr,0x1000,true, sizeof(T_Elf_Ehdr), new_phdr_addr);
		}
		bool CreateNewPhdrs_internal(
			const libIRDB::virtual_offset_t &min_addr, 
			const libIRDB::virtual_offset_t &max_addr,
			const int &first_seg_file_offset,
			const bool &add_pt_load_for_phdr,
			const size_t phdr_map_offset,
			libIRDB::virtual_offset_t new_phdr_addr
			) 
		{
			// create a load segments into the new header list.
			// assume hdr are on first page.
			unsigned int fileoff=first_seg_file_offset;
			for(unsigned int i=0;i<segvec.size();i++)
			{
				T_Elf_Phdr thisphdr;
				memset(&thisphdr,0,sizeof(thisphdr));
			   	thisphdr.p_type = PT_LOAD; 
			   	thisphdr.p_flags = (ELFIO::Elf_Word)segvec[i]->m_perms;      
			   	thisphdr.p_offset = fileoff;     
				std::cout<<"Assigning load[i].ph_offset="<<std::hex<<fileoff<<std::endl;
			   	thisphdr.p_vaddr = (T_Elf_Addr)segvec[i]->start_page; 
			   	thisphdr.p_paddr = (T_Elf_Addr)segvec[i]->start_page; 
			   	thisphdr.p_filesz = (ELFIO::Elf_Xword)segvec[i]->filesz; 
			   	thisphdr.p_memsz = (ELFIO::Elf_Xword)segvec[i]->memsz; 
			   	thisphdr.p_align=  0x1000;

				new_phdrs.push_back(thisphdr);

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

			if(add_pt_load_for_phdr)
			{
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
				new_phdrs.insert(new_phdrs.begin(),newheaderphdr);
			}

			// create the new phdr phdr
			std::cout<<"New phdrs at: "<<std::hex<<new_phdr_addr<<std::endl;
			T_Elf_Phdr newphdrphdr;
			memset(&newphdrphdr,0,sizeof(newphdrphdr));
			newphdrphdr.p_type = PT_PHDR;
			newphdrphdr.p_flags =(ELFIO::Elf_Word)4;
			newphdrphdr.p_offset =phdr_map_offset;
			newphdrphdr.p_vaddr = new_phdr_addr;
			newphdrphdr.p_paddr = new_phdr_addr;
			newphdrphdr.p_filesz =(ELFIO::Elf_Xword)(new_phdrs.size()+1)*GetSegHeaderSize();
			newphdrphdr.p_memsz =(ELFIO::Elf_Xword)(new_phdrs.size()+1)*GetSegHeaderSize();
			newphdrphdr.p_align =0x1000;
			new_phdrs.insert(new_phdrs.begin(),newphdrphdr);

			// record the new ehdr.
			new_ehdr=ehdr;
			new_ehdr.e_phoff=phdr_map_offset;
			new_ehdr.e_shoff=0;
			new_ehdr.e_shnum=0;
			new_ehdr.e_phnum=new_phdrs.size();
			// new_ehdr.e_phoff=sizeof(new_ehdr);
			new_ehdr.e_shstrndx=0;
			return true;
		}
		void WriteElf(FILE* fout)
		{
			assert(fout);

			// write the segments first, as they may overlap the ehdr and phdrs
			// and we maintain those separately.

			// look at enach phdr entry, and write data as it says to.
			unsigned int loadcnt=0;
			for(unsigned int i=0;i<new_phdrs.size();i++)
			{
				if(new_phdrs[i].p_type==PT_LOAD)
				{
					loadcnt++;
				
					std::cout<<"phdr["<<std::dec<<loadcnt<<"] writing at:"<<std::hex<<new_phdrs[i].p_offset<<std::endl;
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
							for (k=PAGE_SIZE-1;k>=0;k--)
							{
								if(page.data[k]!=0)
									break;
							}
							std::cout<<"phdr["<<std::dec<<loadcnt<<"] optimizing last page write to size k="<<std::hex<<k<<std::endl;
							fwrite(page.data.data(), k+1, 1, fout);
						}
				
					}
				}
			}

			// write the header.
			fseek(fout,0,SEEK_SET);
			fwrite(&new_ehdr, sizeof(new_ehdr), 1, fout);
			// write the phdrs, which may be part of a segment written above.
			std::cout<<"Writing segment headers at "<<std::hex<<new_ehdr.e_phoff
				<<", size="<<new_phdrs.size()*sizeof(new_phdrs[0])<<std::endl;
			fseek(fout,new_ehdr.e_phoff,SEEK_SET);
			fwrite(new_phdrs.data(), sizeof(new_phdrs[0]), new_phdrs.size(), fout);
		}

	private:
		unsigned int DetermineMaxPhdrSize()
		{
			unsigned int phdr_count=0;
			/* count phdr's that aren't pt_load or pt_phdr */
			for(unsigned int i=0;i<phdrs.size();i++)
			{
				// skip any load sections, the irdb tells us what to load.
				if(phdrs[i].p_type == PT_LOAD)
					continue;

				// skip phdr section.
				if(phdrs[i].p_type == PT_PHDR)
					continue;
				
				phdr_count++;
			}

			// add a phdr for each segment that needs a mapping
			phdr_count+=segvec.size();
			
			// add 2 more sections that 1) of type PT_PHDR, and 2) a possible PT_LOAD section to load the phdr.
			// worst case is we allocate an entire new page for it.
			phdr_count+=2;		

			return phdr_count*sizeof(T_Elf_Phdr);
		}

		T_Elf_Ehdr ehdr;
		T_Elf_Ehdr new_ehdr;
		std::vector<T_Elf_Phdr> phdrs;
		std::vector<T_Elf_Phdr> new_phdrs;
};

typedef class ElfWriterImpl<ELFIO::Elf64_Ehdr, ELFIO::Elf64_Phdr, ELFIO::Elf64_Addr> ElfWriter64;
typedef class ElfWriterImpl<ELFIO::Elf32_Ehdr, ELFIO::Elf32_Phdr, ELFIO::Elf32_Addr> ElfWriter32;



#endif

