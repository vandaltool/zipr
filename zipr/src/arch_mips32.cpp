#include <zipr_all.h>

namespace zipr
{
#include <arch/arch_mips32.hpp>
}
#include <memory>

using namespace std;
using namespace IRDB_SDK;
using namespace zipr;

#define ALLOF(a) begin(a),end(a)



/*
 * Default way to split a dollop is to create a jump instruction, push it on the dollop, then ask plugins if they want anything to do with it
 */
RangeAddress_t ZiprArchitectureHelperMIPS32_t::splitDollop(FileIR_t* p_firp, Zipr_SDK::Dollop_t* to_split, const RangeAddress_t p_cur_addr)
{
	assert(!to_split->empty());
	const auto last_de             = *to_split->rbegin(); // end of DollopEntryList 
	const auto last_insn           = last_de->getInstruction();
	const auto last_insn_is_branch = DecodedInstruction_t::factory(last_insn)->isBranch();
	const auto nop_bits            = string("\x00\x00\x00\x00",4);
        auto zipr_impl                 = dynamic_cast<ZiprImpl_t*>(m_zipr);
        auto cur_addr                  = p_cur_addr;
        auto fallthrough               = to_split->getFallthroughDollop();

	const auto add_instruction = [&](Instruction_t* patch) -> DollopEntry_t*
		{
			/* need a dollop entry */
			auto patch_de    = new DollopEntry_t(patch, to_split);

			/* place it */
			patch_de->Place(cur_addr);

			/* advance cur_addr */
			cur_addr += m_zipr->determineDollopEntrySize(patch_de, false);

			/* add it to the dollop and placement queue */
			to_split->push_back(patch_de);
			m_zipr->getPlacementQueue()->insert({fallthrough, cur_addr});

			/*
			 * Since we inserted a new instruction, we should
			 * check to see whether a plugin wants to plop it.
			 */
			zipr_impl->AskPluginsAboutPlopping(patch_de->getInstruction());

			return patch_de;
		};


	/* if the dollop ends in some kind of branch we need the delay slot instruction */
	if(last_insn_is_branch)
	{
		const auto is_delay_slot_reloc = [](const Relocation_t* reloc) -> bool  { return reloc->getType() == "delay_slot1"; } ;
		const auto &last_insn_relocs = last_insn->getRelocations();
		const auto delay_slot_insn_it = find_if(ALLOF(last_insn_relocs), is_delay_slot_reloc);
		assert(delay_slot_insn_it != end(last_insn_relocs));
		const auto delay_slot_insn = dynamic_cast<Instruction_t*>((*delay_slot_insn_it)->getWRT());

		auto new_delay_slot = delay_slot_insn == nullptr ? 
			IRDB_SDK::addNewDataBits(p_firp, nullptr, nop_bits) :
			IRDB_SDK::addNewDataBits(p_firp, delay_slot_insn->getTarget(), delay_slot_insn->getDataBits());

		/* add a copy of the delay slot instruction */
		add_instruction(new_delay_slot);
	}

	/* add a jump instruction */
	const auto target_insn = (*fallthrough->begin())->getInstruction()->getFallthrough(); // jumps over delay slot
	add_instruction(createNewJumpInstruction(p_firp, target_insn))->setTargetDollop(fallthrough);

	auto new_branch_delay_slot = IRDB_SDK::addNewDataBits(p_firp, nullptr, nop_bits);

	/* add a copy of the delay slot instruction */
	add_instruction(new_branch_delay_slot);

	/* finally, mark that we've patched this dollop to jump to the target */
	to_split->setFallthroughPatched(true);

	// renew the end of the newly placed instructions.
        return cur_addr;
}
