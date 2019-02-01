

#include <zipr_all.h>

namespace zipr
{
#include <sizer/sizer_x86.hpp>
#include <sizer/sizer_arm64.hpp>
}
#include <memory>

using namespace std;
using namespace IRDB_SDK;
using namespace zipr;


unique_ptr<ZiprSizerBase_t> ZiprSizerBase_t::factory(Zipr_SDK::Zipr_t* p_zipr_obj)
{
	auto l_firp=p_zipr_obj->getFileIR();
	auto ret= l_firp->getArchitecture()->getMachineType() == admtX86_64   ?  (ZiprSizerBase_t*)new ZiprSizerX86_t  (p_zipr_obj) :
	          l_firp->getArchitecture()->getMachineType() == admtI386     ?  (ZiprSizerBase_t*)new ZiprSizerX86_t  (p_zipr_obj) :
	          l_firp->getArchitecture()->getMachineType() == admtAarch64  ?  (ZiprSizerBase_t*)new ZiprSizerARM64_t(p_zipr_obj) :
	          throw domain_error("Cannot init architecture");

	return unique_ptr<ZiprSizerBase_t>(ret);
}

ZiprSizerBase_t::ZiprSizerBase_t(Zipr_SDK::Zipr_t* p_zipr_obj,
		const size_t p_CALLBACK_TRAMPOLINE_SIZE,
		const size_t p_TRAMPOLINE_SIZE,
		const size_t p_LONG_PIN_SIZE,
		const size_t p_SHORT_PIN_SIZE,
		const size_t p_ALIGNMENT
		) :
	memory_space(*dynamic_cast<zipr::ZiprMemorySpace_t*>(p_zipr_obj->GetMemorySpace())),
	m_zipr_obj(*dynamic_cast<zipr::ZiprImpl_t*>(p_zipr_obj)),
	CALLBACK_TRAMPOLINE_SIZE(p_CALLBACK_TRAMPOLINE_SIZE),
	TRAMPOLINE_SIZE         (p_TRAMPOLINE_SIZE         ),
	LONG_PIN_SIZE           (p_LONG_PIN_SIZE           ),
	SHORT_PIN_SIZE          (p_SHORT_PIN_SIZE          ),
	ALIGNMENT               (p_ALIGNMENT               )
{
}


Range_t ZiprSizerBase_t::DoPlacement(const size_t size) const
{
	auto new_place=memory_space.getFreeRange(size+ALIGNMENT-1);	
	auto aligned_start=(new_place.getStart()+ALIGNMENT-1)&~(ALIGNMENT-1);
	return { aligned_start, new_place.getEnd() };
}


size_t ZiprSizerBase_t::DetermineDollopSizeInclFallthrough(Dollop_t *dollop) const
{
        size_t fallthroughs_wcds = 0;
        Dollop_t *fallthrough_it = nullptr;
        for (fallthrough_it = dollop;
             fallthrough_it != nullptr;
                         fallthrough_it = fallthrough_it->FallthroughDollop())
        {
                if (fallthrough_it->IsPlaced())
                        /*
                         * We are going to stop calculating when
                         * we see that we'll hit a dollop that
                         * is already placed.
                         */
                        break;

                fallthroughs_wcds += fallthrough_it->getSize();
                /*
                 * For every dollop that we consolidate,
                 * we will lose TRAMPOLINE_SIZE bytes by
                 * not having to jump to the fallthrough.
                 * That space is included in getSize()
                 * result, so we subtract it here.
                 */
                if (fallthrough_it->FallthroughDollop())
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


