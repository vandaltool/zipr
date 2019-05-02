

#include <zipr_all.h>

namespace zipr
{
#include <arch/arch_x86.hpp>
#include <arch/arch_arm64.hpp>
#include <arch/arch_arm32.hpp>
}
#include <memory>

using namespace std;
using namespace IRDB_SDK;
using namespace zipr;

ZiprArchitectureHelperBase_t::ZiprArchitectureHelperBase_t(Zipr_SDK::Zipr_t* p_zipr_obj) :
	m_pinner (ZiprPinnerBase_t ::factory(p_zipr_obj)),
	m_patcher(ZiprPatcherBase_t::factory(p_zipr_obj)),
	m_sizer  (ZiprSizerBase_t  ::factory(p_zipr_obj)) 
{
}


unique_ptr<ZiprArchitectureHelperBase_t> ZiprArchitectureHelperBase_t::factory(Zipr_SDK::Zipr_t* p_zipr_obj)
{
	auto l_firp=p_zipr_obj->getFileIR();
	auto ret= l_firp->getArchitecture()->getMachineType() == admtX86_64   ?  (ZiprArchitectureHelperBase_t*)new ZiprArchitectureHelperX86_t  (p_zipr_obj) :
	          l_firp->getArchitecture()->getMachineType() == admtI386     ?  (ZiprArchitectureHelperBase_t*)new ZiprArchitectureHelperX86_t  (p_zipr_obj) :
	          l_firp->getArchitecture()->getMachineType() == admtAarch64  ?  (ZiprArchitectureHelperBase_t*)new ZiprArchitectureHelperARM64_t(p_zipr_obj) :
	          l_firp->getArchitecture()->getMachineType() == admtArm32    ?  (ZiprArchitectureHelperBase_t*)new ZiprArchitectureHelperARM32_t(p_zipr_obj) :
	          throw domain_error("Cannot init architecture");

	return unique_ptr<ZiprArchitectureHelperBase_t>(ret);
}
