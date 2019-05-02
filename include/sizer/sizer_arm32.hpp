#ifndef SIZERARM32_HPP
#define SIZERARM32_HPP

class ZiprSizerARM32_t : public ZiprSizerBase_t
{
	private:
		RangeAddress_t TBZPlopDollopEntryWithTarget    (Zipr_SDK::DollopEntry_t *entry, RangeAddress_t override_place, RangeAddress_t override_target) const;
		RangeAddress_t DefaultPlopDollopEntryWithTarget(Zipr_SDK::DollopEntry_t *entry, RangeAddress_t override_place, RangeAddress_t override_target) const;

	public:
		ZiprSizerARM32_t(Zipr_SDK::Zipr_t* p_zipr_obj);
		size_t DetermineInsnSize(IRDB_SDK::Instruction_t*, bool account_for_jump = true) const override;
		virtual RangeAddress_t PlopDollopEntryWithTarget(Zipr_SDK::DollopEntry_t *entry, RangeAddress_t override_place, RangeAddress_t override_target) const override;


};
#endif
