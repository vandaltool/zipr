#ifndef _LIBTRANSFORM_FIX_RETS_H_
#define _LIBTRANSFORM_FIX_RETS_H_

#include "../../libtransform/include/transform.hpp"
#include "../../libMEDSannotation/include/VirtualOffset.hpp"
using namespace std;
using namespace libIRDB;

class FixRets : public libTransform::Transform
{
	public:
		FixRets(FileIR_t*p_variantIR); 
		int execute();
};
#endif
