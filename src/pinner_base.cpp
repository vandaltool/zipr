#include <zipr_all.h>

namespace zipr
{
#include <pinner/pinner_x86.hpp>
#include <pinner/pinner_arm64.hpp>
}
#include <memory>
#include <Rewrite_Utility.hpp>

using namespace std;
using namespace libIRDB;
using namespace zipr;

unique_ptr<ZiprPinnerBase_t> ZiprPinnerBase_t::factory(Zipr_SDK::Zipr_t *p_parent)
{
	auto ret= p_parent->GetFileIR()->GetArchitecture()->getMachineType() == admtX86_64   ?  (ZiprPinnerBase_t*)new ZiprPinnerX86_t  (p_parent) :
	          p_parent->GetFileIR()->GetArchitecture()->getMachineType() == admtI386     ?  (ZiprPinnerBase_t*)new ZiprPinnerX86_t  (p_parent) :
	          p_parent->GetFileIR()->GetArchitecture()->getMachineType() == admtAarch64  ?  (ZiprPinnerBase_t*)new ZiprPinnerARM64_t(p_parent) :
	          throw domain_error("Cannot init architecture");

	return unique_ptr<ZiprPinnerBase_t>(ret);
}
