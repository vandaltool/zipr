#include <zipr_all.h>

namespace zipr
{
#include <sizer/sizer_arm64.hpp>
}


using namespace zipr ;
using namespace IRDB_SDK;


static bool is_tbz_type(Instruction_t* insn)
{
	const auto d=DecodedInstruction_t::factory(insn);
	return d->getMnemonic()=="tbz" || d->getMnemonic()=="tbnz";

}

ZiprSizerARM64_t::ZiprSizerARM64_t(Zipr_SDK::Zipr_t* p_zipr_obj) : ZiprSizerBase_t(p_zipr_obj,4,4,4,4,4)
{
}

size_t ZiprSizerARM64_t::DetermineInsnSize(Instruction_t* insn, bool account_for_jump) const
{
	// tbz plan needs 12 bytes.  other plans need 4
	const auto size = is_tbz_type(insn) ? 12 : 4;
	const auto jmp_size= account_for_jump ? 4 : 0;
	return size + jmp_size;
}

RangeAddress_t ZiprSizerARM64_t::PlopDollopEntryWithTarget(
        Zipr_SDK::DollopEntry_t *entry,
        RangeAddress_t override_place,
        RangeAddress_t override_target) const
{
        assert(entry->getTargetDollop());
	if(is_tbz_type(entry->getInstruction()))
		return  TBZPlopDollopEntryWithTarget(entry,override_place,override_target);
	return  DefaultPlopDollopEntryWithTarget(entry,override_place,override_target);
}

RangeAddress_t ZiprSizerARM64_t::TBZPlopDollopEntryWithTarget(
        Zipr_SDK::DollopEntry_t *entry,
        RangeAddress_t override_place,
        RangeAddress_t override_target) const
{
/* tbz plan:
 * L0  tbz <args>, L2
 * L1  b L3
 * L2: b <target>
 * L3: # fallthrough
 */ 
        const auto addr        = (override_place  == 0) ? entry->getPlace()                 : override_place;
        const auto target_addr = (override_target == 0) ? entry->getTargetDollop()->getPlace() : override_target;
	const auto insn        = entry->getInstruction();
	const auto branch_bytes=string("\x00\x00\x00\x014",4);

	// put the tbz first.
	memory_space.PlopBytes(addr, insn->getDataBits().c_str(), 4);
	m_zipr_obj.GetPatcher()->ApplyPatch(addr,addr+8);// make it jump to L2

	// now drop down a uncond jump for L1, and make it go to L3
	// (i.e., jump around the jump to the target) 
	memory_space.PlopBytes(addr+4, branch_bytes.c_str(), 4);
	m_zipr_obj.GetPatcher()->ApplyPatch(addr+4,addr+12);// make it jump to +8

	// lastly, put down the uncond jump at L2, and make it go to the target 
	memory_space.PlopBytes(addr+8, branch_bytes.c_str(), 4);
	m_zipr_obj.GetPatcher()->ApplyPatch(addr+8,target_addr);// make it jump to +8

	return addr+12; /* address of fallthrough, L3 */

}


RangeAddress_t ZiprSizerARM64_t::DefaultPlopDollopEntryWithTarget(
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

