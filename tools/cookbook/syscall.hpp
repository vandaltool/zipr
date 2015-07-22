#ifndef _LIBTRANSFORM_SYSCALL_H_
#define _LIBTRANSFORM_SYSCALL_H_

#include "cookbook.hpp"
namespace libTransform
{
	using namespace std;
	using namespace libIRDB;

	class Syscall : public CookbookTransform
	{
		public:
			Syscall(VariantID_t *p_variantID,
				FileIR_t*p_variantIR,
				set<std::string> *p_filteredFunctions); 
			int execute();
		private: 
	};
}
#endif
