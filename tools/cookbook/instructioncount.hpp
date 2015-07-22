#ifndef _LIBTRANSFORM_INSTRUCTIONCOUNT_H_
#define _LIBTRANSFORM_INSTRUCTIONCOUNT_H_

#include "cookbook.hpp"

namespace libTransform
{
	using namespace std;
	using namespace libIRDB;

	class InstructionCount : public CookbookTransform
	{
		public:
			InstructionCount(VariantID_t *p_variantID, 
				FileIR_t*p_variantIR,
				set<std::string> *p_filteredFunctions); 

			int execute();
	};
}

#endif
