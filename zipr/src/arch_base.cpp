

#include <zipr_all.h>

namespace zipr
{
#include <arch/arch_x86.hpp>
#include <arch/arch_arm64.hpp>
#include <arch/arch_arm32.hpp>
#include <arch/arch_mips32.hpp>
}
#include <memory>

using namespace std;
using namespace IRDB_SDK;
using namespace zipr;

ZiprArchitectureHelperBase_t::ZiprArchitectureHelperBase_t(Zipr_SDK::Zipr_t* p_zipr_obj) :
	m_pinner (ZiprPinnerBase_t ::factory(p_zipr_obj)),
	m_patcher(ZiprPatcherBase_t::factory(p_zipr_obj)),
	m_sizer  (ZiprSizerBase_t  ::factory(p_zipr_obj)),
	m_zipr   (p_zipr_obj)
{
}


unique_ptr<ZiprArchitectureHelperBase_t> ZiprArchitectureHelperBase_t::factory(Zipr_SDK::Zipr_t* p_zipr_obj)
{
	auto l_firp=p_zipr_obj->getFileIR();
	auto ret= l_firp->getArchitecture()->getMachineType() == admtX86_64   ?  (ZiprArchitectureHelperBase_t*)new ZiprArchitectureHelperX86_t  (p_zipr_obj)  :
	          l_firp->getArchitecture()->getMachineType() == admtI386     ?  (ZiprArchitectureHelperBase_t*)new ZiprArchitectureHelperX86_t  (p_zipr_obj)  :
	          l_firp->getArchitecture()->getMachineType() == admtAarch64  ?  (ZiprArchitectureHelperBase_t*)new ZiprArchitectureHelperARM64_t(p_zipr_obj)  :
	          l_firp->getArchitecture()->getMachineType() == admtArm32    ?  (ZiprArchitectureHelperBase_t*)new ZiprArchitectureHelperARM32_t(p_zipr_obj)  :
	          l_firp->getArchitecture()->getMachineType() == admtMips32   ?  (ZiprArchitectureHelperBase_t*)new ZiprArchitectureHelperMIPS32_t(p_zipr_obj) :
	          throw domain_error("Cannot init architecture");

	return unique_ptr<ZiprArchitectureHelperBase_t>(ret);
}

/*
 * Default way to split a dollop is to create a jump instruction, push it on the dollop, then ask plugins if they want anything to do with it
 */
RangeAddress_t ZiprArchitectureHelperBase_t::splitDollop(FileIR_t* p_firp, Zipr_SDK::Dollop_t* to_split, const RangeAddress_t p_cur_addr)
{
	auto zipr_impl   = dynamic_cast<ZiprImpl_t*>(m_zipr);
	auto cur_addr    = p_cur_addr;
	auto fallthrough = to_split->getFallthroughDollop();
	auto patch       = createNewJumpInstruction(p_firp, nullptr);
	auto patch_de    = new DollopEntry_t(patch, to_split);

	patch_de->setTargetDollop(fallthrough);
	patch_de->Place(cur_addr);
	cur_addr += m_zipr->determineDollopEntrySize(patch_de, false);

	to_split->push_back(patch_de);
	to_split->setFallthroughPatched(true);

	m_zipr->getPlacementQueue()->insert({fallthrough, cur_addr});
	/*
	 * Since we inserted a new instruction, we should
	 * check to see whether a plugin wants to plop it.
	 */
	zipr_impl->AskPluginsAboutPlopping(patch_de->getInstruction());

	return cur_addr;
}
