#include <zipr_all.h>

namespace zipr 
{
#include <sizer/sizer_x86.hpp>
}

using namespace zipr ;
using namespace IRDB_SDK;


ZiprSizerX86_t::ZiprSizerX86_t(Zipr_SDK::Zipr_t* p_zipr_obj) : ZiprSizerBase_t(p_zipr_obj,10,5,2,5,1)
{
}

size_t ZiprSizerX86_t::DetermineInsnSize(Instruction_t* insn, bool account_for_jump) const
{

	int required_size=0;

	switch(insn->getDataBits()[0])
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
			required_size=insn->getDataBits().size();
			if (insn->getCallback()!="") required_size=CALLBACK_TRAMPOLINE_SIZE;
			break;
		}
	}
	
	// add an extra 5 for a "trampoline" in case we have to end this fragment early
	if (account_for_jump)
		return required_size+TRAMPOLINE_SIZE;
	else
		return required_size;
}

RangeAddress_t ZiprSizerX86_t::PlopDollopEntryWithTarget(
        Zipr_SDK::DollopEntry_t *entry,
        RangeAddress_t override_place,
        RangeAddress_t override_target) const
{
        auto insn = entry->getInstruction();

        assert(entry->getTargetDollop());

        auto addr = entry->getPlace();
        auto target_addr = entry->getTargetDollop()->getPlace();
        auto ret = addr;

        if (override_place != 0)
                addr = ret = override_place;

        if (override_target != 0)
                target_addr = override_target;

        if(insn->getDataBits().length() >2)
        {
                memory_space.plopBytes(ret,
                                       insn->getDataBits().c_str(),
                                       insn->getDataBits().length()
                                      );
                m_zipr_obj.GetPatcher()->ApplyPatch(ret, target_addr);
                ret+=insn->getDataBits().length();
                return ret;
        }

        // call, jmp, jcc of length 2.
        char b=insn->getDataBits()[0];
        switch(b)
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
                // two byte JCC
                        char bytes[]={(char)0x0f,(char)0xc0,(char)0x0,(char)0x0,(char)0x0,(char)0x0 };  // 0xc0 is a placeholder, overwritten next statement
                        bytes[1]=insn->getDataBits()[0]+0x10;           // convert to jcc with 4-byte offset.
                        memory_space.plopBytes(ret,bytes, sizeof(bytes));
                        m_zipr_obj.GetPatcher()->ApplyPatch(ret, target_addr);
                        ret+=sizeof(bytes);
                        return ret;
                }
                case (char)0xeb:
                {
                        // two byte JMP
                        char bytes[]={(char)0xe9,(char)0x0,(char)0x0,(char)0x0,(char)0x0 };
                        bytes[1]=insn->getDataBits()[0]+0x10;           // convert to jcc with 4-byte offset.
                        memory_space.plopBytes(ret,bytes, sizeof(bytes));
                        m_zipr_obj.GetPatcher()->ApplyPatch(ret, target_addr);
                        ret+=sizeof(bytes);
                        return ret;
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
                        char bytes[]={0,0x5};
                        auto fallthrough_de = entry->getMemberOfDollop()->getFallthroughDollopEntry(entry);

                        /*
                         * This means that we have a fallthrough for this dollop entry
                         * that is actually the fallthrough of the dollop! Wowser.
                         * TODO: Before doing this, check to make sure that _entry_
                         * is the last of the dollop.
                         */
                        if (!fallthrough_de)
                        {
                                if(entry->getMemberOfDollop()->getFallthroughDollop())
                                        fallthrough_de = entry->getMemberOfDollop()->getFallthroughDollop()->front();
                                else
                                        // even a cond branch may have a null fallthrough, account for that here
                                        // by plopping nothing
                                        return ret;
                        }

                        assert(fallthrough_de && fallthrough_de->isPlaced());

                        bytes[0]=insn->getDataBits()[0];
                        memory_space.plopBytes(ret,bytes, sizeof(bytes));
                        ret+=sizeof(bytes);

                        memory_space.plopJump(ret);
                        m_zipr_obj.GetPatcher()->ApplyPatch(ret, fallthrough_de->getPlace());
                        ret+=5;

                        memory_space.plopJump(ret);
                        m_zipr_obj.GetPatcher()->ApplyPatch(ret, target_addr);
                        ret+=5;

                        return ret;

                }

                default:
                        assert(0);
        }
}




