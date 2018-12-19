

#include <zipr_all.h>

namespace zipr
{
#include <arch/archX86.hpp>
#include <arch/archARM64.hpp>
}
#include <memory>
#include <Rewrite_Utility.hpp>

using namespace std;
using namespace libIRDB;
using namespace zipr;

unique_ptr<ZiprArchitectureHelperBase_t> ZiprArchitectureHelperBase_t::factory(FileIR_t* p_firp)
{
	auto ret= p_firp->GetArchitecture()->getMachineType() == admtX86_64   ?  (ZiprArchitectureHelperBase_t*)new ZiprArchitectureHelperX86_t  () :
	          p_firp->GetArchitecture()->getMachineType() == admtI386     ?  (ZiprArchitectureHelperBase_t*)new ZiprArchitectureHelperX86_t  () :
	          p_firp->GetArchitecture()->getMachineType() == admtAarch64  ?  (ZiprArchitectureHelperBase_t*)new ZiprArchitectureHelperARM64_t() :
	          throw domain_error("Cannot init architecture");

	return unique_ptr<ZiprArchitectureHelperBase_t>(ret);
}
