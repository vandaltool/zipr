#ifndef _LIBTRANSFORM_FUNCTIONCALL_H_
#define _LIBTRANSFORM_FUNCTIONCALL_H_

#include "cookbook.hpp"
namespace libTransform
{
	using namespace std;
	using namespace libIRDB;

	class Functioncall : public CookbookTransform
	{
		public:
			Functioncall(VariantID_t *p_variantID,
				FileIR_t*p_variantIR,
				set<std::string> *p_filteredFunctions); 
			int execute();
		private: 
	};
}
#endif
