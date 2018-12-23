#ifndef SIZERARM64_HPP
#define SIZERARM64_HPP

class ZiprSizerARM64_t : public ZiprSizerBase_t
{
	public:
		ZiprSizerARM64_t(Zipr_SDK::Zipr_t* p_zipr_obj);
                size_t DetermineInsnSize(libIRDB::Instruction_t*, bool account_for_jump = true) const override;

};
#endif
