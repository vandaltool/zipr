#include <zipr_all.h>

namespace zipr
{
#include <pinner/pinner_mips32.hpp>
}
#include <memory>

using namespace std;
using namespace IRDB_SDK;
using namespace zipr;

ZiprPinnerMIPS32_t::ZiprPinnerMIPS32_t(Zipr_SDK::Zipr_t* p_parent) :
	m_parent(dynamic_cast<zipr::ZiprImpl_t*>(p_parent)),     // upcast to ZiprImpl
	m_firp(p_parent->getFileIR()),
	memory_space(*m_parent->getMemorySpace()),
	m_dollop_mgr(*p_parent->getDollopManager()),
        placement_queue(*p_parent->getPlacementQueue())

{
}

void  ZiprPinnerMIPS32_t::doPinning()
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
                        auto newDoll=m_dollop_mgr.addNewDollops(insn);
                        placement_queue.insert({newDoll, 0});
                        continue;
                }

		auto ibta_addr=(RangeAddress_t)insn-> getIndirectBranchTargetAddress()-> getVirtualOffset();

		// put branch with 16-bit offset in memory
		// i.e., beq $0, $0, imm16
		// including putting a nop in the delay slot.
                uint8_t bytes[]={'\x10','\x00','\x00','\x00', '\x00', '\x00', '\x00', '\x00'};
		for(auto i=0U;i<sizeof(bytes);i++)
		{
			const auto ibta_byte_addr = ibta_addr+i;
			if(memory_space.find(ibta_byte_addr) != memory_space.end() )
			{
				cout << "Byte is marked as both code and data at: " << hex << ibta_byte_addr << endl;
				exit(10);
			}
			memory_space[ibta_byte_addr]=bytes[i];
			memory_space.splitFreeRange(ibta_byte_addr);
		}
		// insert a patch to patch the branch later.
		const auto patch=Patch_t(ibta_addr, UnresolvedType_t::UncondJump_rel26);
		const auto uu=UnresolvedUnpinned_t(insn);
		m_parent->AddPatch(uu,patch);
        }
}
