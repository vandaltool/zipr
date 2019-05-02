#ifndef ARCHARM32_HPP
#define ARCHARM32_HPP

class ZiprArchitectureHelperARM32_t : public ZiprArchitectureHelperBase_t
{
	public:
		ZiprArchitectureHelperARM32_t(Zipr_SDK::Zipr_t* p_zipr_obj) : ZiprArchitectureHelperBase_t(p_zipr_obj)
		{
		}

		virtual IRDB_SDK::Instruction_t* createNewJumpInstruction(IRDB_SDK::FileIR_t *p_firp, IRDB_SDK::Instruction_t* p_existing) override
		{
			// An arm32 brand in binary is:  cond 1 0 1 L signed_immed_24
			//
			// where cond is the 4-bit condition field.  for an uncond branch, we want cond=0b1110
			// and L is whether the link register is set.
			//
			// so, bytes = 0b11101010 0b00000000 0b00000000 0b00000000
			//
			const auto bits =string("\x00\x00\x00\x0ea",4);
			auto ret=IRDB_SDK::addNewDataBits(p_firp, p_existing, bits);
			const auto d=IRDB_SDK::DecodedInstruction_t::factory(ret);
			assert(d->valid());
			return ret;
		}

		virtual IRDB_SDK::Instruction_t* createNewHaltInstruction(IRDB_SDK::FileIR_t *p_firp, IRDB_SDK::Instruction_t* p_existing) override
		{
			// for arm32, we are using a mov pc, #0 as a halt insn.
			// 0xe3a0f000
			const auto bits =string("\x00\xf0\xa0\xe3",4);
			auto ret=IRDB_SDK::addNewDataBits(p_firp, p_existing, bits);
			const auto d=IRDB_SDK::DecodedInstruction_t::factory(ret);
			assert(d->valid());
			return ret;
		}
};
#endif
