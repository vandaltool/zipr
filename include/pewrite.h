
#ifndef PeWriter_h
#define PeWriter_h

#include <vector>
#include <map>

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif


class PeWriter
{
	public: 
		PeWriter(IRDB_SDK::FileIR_t* firp, bool write_sections, bool bss_opts) : m_firp(firp), m_write_sections(write_sections), m_bss_opts(bss_opts) { }
		virtual ~PeWriter() {}
		void Write(const EXEIO::exeio *exeiop, const std::string &out_file, const std::string &infile);

	protected:

		FileIR_t* m_firp=nullptr;
		bool m_write_sections=false;
		bool m_bss_opts=false;



		
};


// 


template <int bitwidth>
class PeWriterImpl : public PeWriter
{
	public:
		PeWriterImpl(IRDB_SDK::FileIR_t* firp, bool write_sections, bool bss_opts) 
			: PeWriter(firp,write_sections,bss_opts)
		{
		}
};

using PeWriter64 = PeWriterImpl<64>;



#endif

