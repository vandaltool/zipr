#include <zipr_all.h>

namespace zipr
{
#include <pinner/pinner_x86.hpp>
#include <pinner/pinner_arm32.hpp>
#include <pinner/pinner_arm64.hpp>
}
#include <memory>

using namespace std;
using namespace IRDB_SDK;
using namespace zipr;

unique_ptr<ZiprPinnerBase_t> ZiprPinnerBase_t::factory(Zipr_SDK::Zipr_t *p_parent)
{
	auto ret= p_parent->getFileIR()->getArchitecture()->getMachineType() == admtX86_64   ?  (ZiprPinnerBase_t*)new ZiprPinnerX86_t  (p_parent) :
	          p_parent->getFileIR()->getArchitecture()->getMachineType() == admtI386     ?  (ZiprPinnerBase_t*)new ZiprPinnerX86_t  (p_parent) :
	          p_parent->getFileIR()->getArchitecture()->getMachineType() == admtAarch64  ?  (ZiprPinnerBase_t*)new ZiprPinnerARM64_t(p_parent) :
	          p_parent->getFileIR()->getArchitecture()->getMachineType() == admtArm32    ?  (ZiprPinnerBase_t*)new ZiprPinnerARM32_t(p_parent) :
	          throw domain_error("Cannot init architecture");

	return unique_ptr<ZiprPinnerBase_t>(ret);
}
