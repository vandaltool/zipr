#include <zipr_all.h>

namespace zipr
{
#include <pinner/pinner_arm64.hpp>
}
#include <memory>

using namespace std;
using namespace IRDB_SDK;
using namespace zipr;

ZiprPinnerARM64_t::ZiprPinnerARM64_t(Zipr_SDK::Zipr_t* p_parent) :
	m_parent(dynamic_cast<zipr::ZiprImpl_t*>(p_parent)),     // upcast to ZiprImpl
	m_firp(p_parent->getFileIR()),
	memory_space(*m_parent->GetMemorySpace()),
	m_dollop_mgr(*p_parent->getDollopManager()),
        placement_queue(*p_parent->GetPlacementQueue())

{
}

void  ZiprPinnerARM64_t::doPinning()
{


	// deal with unpinned IBTs by putting them in the placement queue.
	for(auto &insn : m_firp->getInstructions())
	{
                if(insn->getIndirectBranchTargetAddress()==nullptr)
                        continue;

                if(insn->getIndirectBranchTargetAddress()->getVirtualOffset()==0)
                {
                        // Unpinned IBT. Create dollop and add it to placement
                        // queue straight away--there are no pinning considerations.
                        auto newDoll=m_dollop_mgr.AddNewDollops(insn);
                        placement_queue.insert(pair<Dollop_t*,RangeAddress_t>(newDoll, 0));
                        continue;
                }

		auto ibta_addr=(RangeAddress_t)insn-> getIndirectBranchTargetAddress()-> getVirtualOffset();

		// put unconditional branch with 26-bit offset in memory
		// 0001 0100  0000 0000  0000 0000  0000 0000 
                uint8_t bytes[]={'\x00','\x00','\x00','\x14'};
		for(auto i=0U;i<sizeof(bytes);i++)
		{
			assert(memory_space.find(ibta_addr+i) == memory_space.end() );
			memory_space[ibta_addr+i]=bytes[i];
			memory_space.SplitFreeRange(ibta_addr+i);
		}
		// insert a patch to patch the branch later.
		const auto patch=Patch_t(ibta_addr, UnresolvedType_t::UncondJump_rel26);

		const auto uu=UnresolvedUnpinned_t(insn);
		m_parent->AddPatch(uu,patch);
        }
}
