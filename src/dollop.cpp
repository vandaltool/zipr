#include <zipr_all.h>

namespace Zipr_SDK {
	using namespace libIRDB;
	using namespace zipr;
	Dollop_t::Dollop_t(Instruction_t *start)
	{
		m_start = start;
		m_size = CalculateWorstCaseSize();
	}

	size_t Dollop_t::CalculateWorstCaseSize()
	{
		size_t dollop_size = 0;
		Instruction_t *cur_insn = m_start;
		while (cur_insn != NULL)
		{
			/*
			 * TODO: Take into consideration the dollop might
			 * be MUCH shorter if it is going to jump to a 
			 * previously placed dollop at this insn.
			 */
	#if 0
			if ((to_addr=final_insn_locations[cur_insn]) != 0)
			{
				if (!m_replop)
				{
					if (m_verbose)
						printf("Fallthrough loop detected. ");
					break;
				}
			}
	#endif
			dollop_size += Utils::DetermineWorstCaseInsnSize(cur_insn);
			cur_insn=cur_insn->GetFallthrough();
		}
		return dollop_size;
	}
}
