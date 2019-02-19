#ifndef sd_filebased_h
#define sd_filebased_h

#include <string>
#include <structured_diversity.h>

namespace libStructDiv
{

using namespace std;


class FileBased_StructuredDiversity_t : public StructuredDiversity_t
{
	public:

		FileBased_StructuredDiversity_t(string key, int varid, int total_variants, string p_config);


	protected:
		virtual vector<string> DoBarrier(string value);	

		string m_shared_dir;
		int m_barrier_count;
	
};

}

#endif
