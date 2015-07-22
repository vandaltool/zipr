#ifndef _LIBTRANSFORM_WHITELISTCALL_H_
#define _LIBTRANSFORM_WHITELISTCALL_H_

#include "cookbook.hpp"
namespace libTransform
{
	using namespace std;
	using namespace libIRDB;

	class Whitelistcall : public CookbookTransform
	{
		public:
			Whitelistcall(VariantID_t *p_variantID,
				FileIR_t*p_variantIR,
				set<std::string> *p_filteredFunctions); 
			int execute();
		private: 
	};
}
#endif
