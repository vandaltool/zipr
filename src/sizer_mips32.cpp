#include <zipr_all.h>

namespace zipr
{
#include <sizer/sizer_mips32.hpp>
}


using namespace zipr ;
using namespace IRDB_SDK;


ZiprSizerMIPS32_t::ZiprSizerMIPS32_t(Zipr_SDK::Zipr_t* p_zipr_obj) : ZiprSizerBase_t(p_zipr_obj,4,4,4,4,4)
{
}

size_t ZiprSizerMIPS32_t::DetermineInsnSize(Instruction_t* insn, bool account_for_jump) const
{
	// need 4 bytes for insn, plus 4 bytes for a jump 
	const auto size     = 4u;
	const auto jmp_size = account_for_jump ? 4 : 0;
	return size + jmp_size;
}

RangeAddress_t ZiprSizerMIPS32_t::PlopDollopEntryWithTarget(
        Zipr_SDK::DollopEntry_t *entry,
        RangeAddress_t override_place,
        RangeAddress_t override_target) const
{
        const auto addr        = (override_place  == 0) ? entry->getPlace()                 : override_place;
        const auto target_addr = (override_target == 0) ? entry->getTargetDollop()->getPlace() : override_target;
	const auto insn        = entry->getInstruction();

	// plop instruction an d make it target the right address.
	memory_space.PlopBytes(addr, insn->getDataBits().c_str(), 4);
	m_zipr_obj.GetPatcher()->ApplyPatch(addr, target_addr);

	return addr+4;
}

