

#include <zipr_all.h>

namespace zipr
{
#include <sizer/sizer_x86.hpp>
#include <sizer/sizer_arm64.hpp>
#include <sizer/sizer_arm32.hpp>
#include <sizer/sizer_mips32.hpp>
}
#include <memory>

using namespace std;
using namespace IRDB_SDK;
using namespace zipr;


unique_ptr<ZiprSizerBase_t> ZiprSizerBase_t::factory(Zipr_SDK::Zipr_t* p_zipr_obj)
{
	auto l_firp=p_zipr_obj->getFileIR();
	auto ret= l_firp->getArchitecture()->getMachineType() == admtX86_64   ?  (ZiprSizerBase_t*)new ZiprSizerX86_t  (p_zipr_obj)  :
	          l_firp->getArchitecture()->getMachineType() == admtI386     ?  (ZiprSizerBase_t*)new ZiprSizerX86_t  (p_zipr_obj)  :
	          l_firp->getArchitecture()->getMachineType() == admtAarch64  ?  (ZiprSizerBase_t*)new ZiprSizerARM64_t(p_zipr_obj)  :
	          l_firp->getArchitecture()->getMachineType() == admtArm32    ?  (ZiprSizerBase_t*)new ZiprSizerARM32_t(p_zipr_obj)  :
	          l_firp->getArchitecture()->getMachineType() == admtMips32   ?  (ZiprSizerBase_t*)new ZiprSizerMIPS32_t(p_zipr_obj) :
	          throw domain_error("Cannot init architecture");

	return unique_ptr<ZiprSizerBase_t>(ret);
}

ZiprSizerBase_t::ZiprSizerBase_t(Zipr_SDK::Zipr_t* p_zipr_obj,
		const size_t p_CALLBACK_TRAMPOLINE_SIZE,
		const size_t p_TRAMPOLINE_SIZE,
		const size_t p_LONG_PIN_SIZE,
		const size_t p_SHORT_PIN_SIZE,
		const size_t p_ALIGNMENT,
		const size_t p_UNPIN_ALIGNMENT
		) :
	memory_space(*dynamic_cast<zipr::ZiprMemorySpace_t*>(p_zipr_obj->getMemorySpace())),
	m_zipr_obj(*dynamic_cast<zipr::ZiprImpl_t*>(p_zipr_obj)),
	CALLBACK_TRAMPOLINE_SIZE(p_CALLBACK_TRAMPOLINE_SIZE),
	TRAMPOLINE_SIZE         (p_TRAMPOLINE_SIZE         ),
	LONG_PIN_SIZE           (p_LONG_PIN_SIZE           ),
	SHORT_PIN_SIZE          (p_SHORT_PIN_SIZE          ),
	ALIGNMENT               (p_ALIGNMENT               ),
	UNPIN_ALIGNMENT         (p_UNPIN_ALIGNMENT         )
{
}


Range_t ZiprSizerBase_t::DoPlacement(const size_t p_size, const Zipr_SDK::Dollop_t* p_dollop) const
{
	assert(p_dollop->getSize() > 0);
	const auto dollop_entry        = p_dollop->front();
	const auto dollop_insn         = dollop_entry->getInstruction();
	const auto dollop_ibta         = dollop_insn->getIndirectBranchTargetAddress();
	const auto isUnpinnedIbta      = dollop_ibta ? dollop_ibta->getVirtualOffset() == 0 : false;
	const auto sizeToAlloc         = isUnpinnedIbta ? (p_size+UNPIN_ALIGNMENT-1) : (p_size+ALIGNMENT-1);	
	const auto new_place           = memory_space.getFreeRange(sizeToAlloc);
	const auto aligned_start_nopin = (new_place.getStart()+ALIGNMENT-1)&~(ALIGNMENT-1);
	const auto aligned_start_unpin = (new_place.getStart()+UNPIN_ALIGNMENT-1)&~(UNPIN_ALIGNMENT-1);
	const auto aligned_start       = isUnpinnedIbta ? aligned_start_unpin : aligned_start_nopin;

	return { aligned_start, new_place.getEnd() };
}


size_t ZiprSizerBase_t::DetermineDollopSizeInclFallthrough(Zipr_SDK::Dollop_t *dollop) const
{
        auto fallthroughs_wcds = 0u;
	auto fallthrough_it = dollop;
        for ( /* empty */ ; fallthrough_it != nullptr; fallthrough_it = fallthrough_it->getFallthroughDollop())
        {

                fallthroughs_wcds += fallthrough_it->getSize();

		// if we can't coalesce the next dollop, we can stop now.
		if(!Dollop_t::canCoalesce(fallthrough_it, fallthrough_it->getFallthroughDollop()) )
			break;

		// or, if the next dollop is already placed, we keep the trampolining.
                if (fallthrough_it->isPlaced())
                        /*
                         * We are going to stop calculating when
                         * we see that we'll hit a dollop that
                         * is already placed.
                         */
                        break;

		// otherwise, we can get ride of the trampolining, and add the size of the next dollop.
                /*
                 * For every dollop that we consolidate,
                 * we will lose TRAMPOLINE_SIZE bytes by
                 * not having to jump to the fallthrough.
                 * That space is included in getSize()
                 * result, so we subtract it here.
                 */
                if (fallthrough_it->getFallthroughDollop())
                        fallthroughs_wcds -= TRAMPOLINE_SIZE;

        }
        /*
         * If there is a fallthrough_it, that means
         * that the fallthrough dollop would not be
         * placed again. Instead, we would jump to
         * it. So, we add back in a trampoline.
         */
        if (fallthrough_it)
                fallthroughs_wcds += TRAMPOLINE_SIZE;

        return fallthroughs_wcds;
}


