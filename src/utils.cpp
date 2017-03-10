#include <zipr_all.h>

namespace zipr {
namespace Utils {

void PrintStat(std::ostream &out, std::string description, double value)
{
	out << description << ": " << std::dec << value << std::endl;
}

size_t CALLBACK_TRAMPOLINE_SIZE=10;
size_t TRAMPOLINE_SIZE=5;
size_t SHORT_PIN_SIZE=2;
size_t LONG_PIN_SIZE=5;
using namespace libIRDB;

size_t DetermineWorstCaseDollopSizeInclFallthrough(Dollop_t *dollop)
{	
	size_t fallthroughs_wcds = 0;
	Dollop_t *fallthrough_it = NULL; 
	for (fallthrough_it = dollop;
	     fallthrough_it != NULL;
			 fallthrough_it = fallthrough_it->FallthroughDollop())
	{
		if (fallthrough_it->IsPlaced())
			/*
			 * We are going to stop calculating when
			 * we see that we'll hit a dollop that
			 * is already placed.
			 */
			break;

		fallthroughs_wcds += fallthrough_it->GetSize();
		/*
		 * For every dollop that we consolidate,
		 * we will lose TRAMPOLINE_SIZE bytes by
		 * not having to jump to the fallthrough.
		 * That space is included in GetSize()
		 * result, so we subtract it here.
		 */
		if (fallthrough_it->FallthroughDollop())
			fallthroughs_wcds -= Utils::TRAMPOLINE_SIZE;
	}
	/*
	 * If there is a fallthrough_it, that means
	 * that the fallthrough dollop would not be
	 * placed again. Instead, we would jump to 
	 * it. So, we add back in a trampoline.
	 */
	if (fallthrough_it)
		fallthroughs_wcds += Utils::TRAMPOLINE_SIZE;

	return fallthroughs_wcds;
}

size_t DetermineWorstCaseInsnSize(Instruction_t* insn, bool account_for_jump)
{

	int required_size=0;

	switch(insn->GetDataBits()[0])
	{
		case (char)0x70:
		case (char)0x71:
		case (char)0x72:
		case (char)0x73:
		case (char)0x74:
		case (char)0x75:
		case (char)0x76:
		case (char)0x77:
		case (char)0x78:
		case (char)0x79:
		case (char)0x7a:
		case (char)0x7b:
		case (char)0x7c:
		case (char)0x7d:
		case (char)0x7e:
		case (char)0x7f:
		{
			// two byte JCC -> 6byte JCC
			required_size=6;
			break;
		}

		case (char)0xeb:
		{
			// two byte JMP -> 5byte JMP
			required_size=5;
			break;
		}

		case (char)0xe0:
		case (char)0xe1:
		case (char)0xe2:
		case (char)0xe3:
		{
			// loop, loopne, loopeq, jecxz
			// convert to:
			// <op> +5:
			// jmp fallthrough
			// +5: jmp target
			// 2+5+5;
			required_size=12;
			break;
		}
		

		default:
		{
			required_size=insn->GetDataBits().size();
			if (insn->GetCallback()!="") required_size=CALLBACK_TRAMPOLINE_SIZE;
			break;
		}
	}
	
	// add an extra 5 for a "trampoline" in case we have to end this fragment early
	if (account_for_jump)
		return required_size+TRAMPOLINE_SIZE;
	else
		return required_size;
}
}
}
