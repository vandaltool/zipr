#ifndef SIZERARM64_HPP
#define SIZERARM64_HPP

class ZiprSizerARM64_t : public ZiprSizerBase_t
{
	private:
	RangeAddress_t TBZPlopDollopEntryWithTarget    (DollopEntry_t *entry, RangeAddress_t override_place, RangeAddress_t override_target) const;
	RangeAddress_t DefaultPlopDollopEntryWithTarget(DollopEntry_t *entry, RangeAddress_t override_place, RangeAddress_t override_target) const;

	public:
	ZiprSizerARM64_t(Zipr_SDK::Zipr_t* p_zipr_obj);
	size_t DetermineInsnSize(libIRDB::Instruction_t*, bool account_for_jump = true) const override;
	virtual RangeAddress_t PlopDollopEntryWithTarget(DollopEntry_t *entry, RangeAddress_t override_place, RangeAddress_t override_target) const override;


};
#endif
