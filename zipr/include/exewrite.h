
#ifndef ExeWriter_h
#define ExeWriter_h

#include <vector>
#include <map>


class ExeWriter
{
	protected: 


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


		uint64_t filesz; 
		uint64_t memsz; 
		uint64_t filepos;
		uint64_t start_page;
		uint64_t m_perms;
			
	};
	using LoadSegmentVector_t = std::vector<LoadSegment_t*>;
	using PageMap_t           = std::map<IRDB_SDK::VirtualOffset_t, PageData_t>;

	public: 
		ExeWriter(EXEIO::exeio* p_exeiop, IRDB_SDK::FileIR_t* firp, bool write_sections, bool bss_opts) 
			: 
				m_exeiop(p_exeiop), 
				m_firp(firp), 
				m_write_sections(write_sections), 
				m_bss_opts(bss_opts) 
		{ 
		}

		virtual ~ExeWriter() {}
		virtual void Write(const std::string &out_file, const std::string &infile) = 0;

	protected:

		EXEIO::exeio* m_exeiop;
		IRDB_SDK::FileIR_t* m_firp;
		bool m_write_sections;
		bool m_bss_opts;
		PageMap_t pagemap;
		LoadSegmentVector_t segvec;

#ifndef PAGE_SIZE
		const int PAGE_SIZE=4096;
#endif
		template <class T> 
		static T page_align(const T& in)
		{

			return in&~(PAGE_SIZE-1);
		}
		template <class T> 
		static inline T page_round_down(const T& x)
		{
			return round_down_to(x,PAGE_SIZE);
		}
		template <class T> 
		static inline T page_round_up(const T& x)
		{
			return round_up_to(x,PAGE_SIZE);
		}
		template <class T> 
		static inline T round_down_to(const T& x, const uint64_t& to)
		{
			assert( (to & (to-1)) == 0 );
			return x & (~(to-1));
		}
		template <class T> 
		static inline T round_up_to(const T& x, const uint64_t& to)
		{
			assert( (to & (to-1)) == 0 );
			return  ( (((uintptr_t)(x)) + to-1)  & (~(to-1)) );
		}

		IRDB_SDK::VirtualOffset_t DetectMinAddr();
		IRDB_SDK::VirtualOffset_t DetectMaxAddr();

		void CreatePagemap();
		void CreateSegmap();
		void SortSegmap();

};


#endif

