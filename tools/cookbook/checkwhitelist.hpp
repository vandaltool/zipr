#ifndef _LIBTRANSFORM_CHECKWHITELIST_H_
#define _LIBTRANSFORM_CHECKWHITELIST_H_

#include "cookbook.hpp"
namespace libTransform
{
	using namespace std;
	using namespace libIRDB;

	class Checkwhitelist : public CookbookTransform
	{
		public:
			Checkwhitelist(VariantID_t *p_variantID,
				FileIR_t*p_variantIR,
				set<std::string> *p_filteredFunctions); 
			int execute();
		private: 
	};
}
#endif
