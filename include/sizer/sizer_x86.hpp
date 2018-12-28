#ifndef SIZERX86_HPP
#define SIZERX86_HPP


class ZiprSizerX86_t  : public ZiprSizerBase_t
{
	public:
		ZiprSizerX86_t(Zipr_SDK::Zipr_t* p_zipr_obj);
                size_t DetermineInsnSize(libIRDB::Instruction_t*, bool account_for_jump = true) const override;
		virtual RangeAddress_t PlopDollopEntryWithTarget( DollopEntry_t *entry, RangeAddress_t override_place, RangeAddress_t override_target) const override;


};
#endif
