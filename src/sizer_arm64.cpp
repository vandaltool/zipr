#include <zipr_all.h>

namespace zipr
{
#include <sizer/sizer_arm64.hpp>
}


using namespace zipr ;


ZiprSizerARM64_t::ZiprSizerARM64_t(Zipr_SDK::Zipr_t* p_zipr_obj) : ZiprSizerBase_t(p_zipr_obj,4,4,4,4,4)
{
}

size_t ZiprSizerARM64_t::DetermineInsnSize(Instruction_t* insn, bool account_for_jump) const
{
	return account_for_jump ? 8 : 4;
}
