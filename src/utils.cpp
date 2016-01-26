#include <zipr_all.h>

namespace zipr {
namespace Utils {

void PrintStat(std::ostream &out, std::string description, double value)
{
	out << description << ": " << std::dec << value << std::endl;
}

size_t CALLBACK_TRAMPOLINE_SIZE=9;
size_t TRAMPOLINE_SIZE=5;
using namespace libIRDB;
int DetermineWorstCaseInsnSize(Instruction_t* insn, bool account_for_jump)
{

	int required_size=0;

	switch(insn->GetDataBits()[0])
	{
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
			if (insn->GetCallback()!="") required_size+=CALLBACK_TRAMPOLINE_SIZE;
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
