#ifndef SD_SDBASE_H
#define SD_SDBASE_H

#include <string>
#include <vector>
#include <assert.h>
#include <sstream>

namespace libStructDiv
{

using namespace std;


class StructuredDiversity_t
{
	public:

		// STatic factory
		static StructuredDiversity_t* factory(string key, string p_ipc_config);

	
		// cannot use directly since there are abstract methods.
		// use from children classes.
		StructuredDiversity_t(string p_key, int p_variant_num, int p_total_variants) : 
			m_variant_num(p_variant_num), m_total_variants(p_total_variants), m_key(p_key)
		{
			// nothing else to do.
		}


		// Perform a barrier of type T.  T should be marshable
		// via the << and >> operators.  To avoid this, the caller can self-marshal 
		// (Will recommends a protocol buffer package for self-marshalling)
		// and invoke Barrier with a string.
		template<class T> vector<T> Barrier(T value)
		{
        		/* marshal value into stream s */
        		stringstream s;
        		s<<value;
		
        		// pass marshalled value to DoBarrier
        		const vector<string> &string_res=DoBarrier(s.str());
		
        		assert(string_res.size()==m_total_variants);
		
        		/* declare a result */
        		vector<T> t_res;
		
        		// for each item 
        		for(int i=0;i<m_total_variants;i++)
        		{
                		stringstream t;
                		T res;
                		t.str(string_res[i]);
                		t>>res;
                		t_res.push_back(res);
        		}
		
        		return t_res;
		}



		// Getters that most cfar things will need.
		int GetVariantID() const { return m_variant_num; }
		int GetNumberOfVariants() const { return m_total_variants; }
		string GetKey() const { return m_key; }

	protected:
		virtual vector<string> DoBarrier(string value)=0;

		int m_variant_num;
		int m_total_variants;
		string m_key;
	
};

}
#endif
