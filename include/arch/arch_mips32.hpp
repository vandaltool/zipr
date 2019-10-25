#ifndef ARCHMIPS32_HPP
#define ARCHMIPS32_HPP

class ZiprArchitectureHelperMIPS32_t : public ZiprArchitectureHelperBase_t
{
	public:
		ZiprArchitectureHelperMIPS32_t(Zipr_SDK::Zipr_t* p_zipr_obj) : ZiprArchitectureHelperBase_t(p_zipr_obj)
		{
		}

		virtual IRDB_SDK::Instruction_t* createNewJumpInstruction(IRDB_SDK::FileIR_t *p_firp, IRDB_SDK::Instruction_t* p_existing) override
		{
			// bytes = 0b00000000 0b00000000 0b00000000 0b00010000
			// jump always with 26bit offset
			const auto bits =string("\x10\x00\x00\x000",4);
			auto ret=IRDB_SDK::addNewDataBits(p_firp, p_existing, bits);
			const auto d=IRDB_SDK::DecodedInstruction_t::factory(ret);
			assert(d->valid());
			return ret;
		}

		virtual IRDB_SDK::Instruction_t* createNewHaltInstruction(IRDB_SDK::FileIR_t *p_firp, IRDB_SDK::Instruction_t* p_existing) override
		{
			// 0b00000000 0b00000000 0b00000000 0b00110100
			// teq $0, $0, 0 --> trap always (as $0 is always 0)
			const auto bits = string("\x00\x00\x00\x34",4);
			auto ret = IRDB_SDK::addNewDataBits(p_firp, p_existing, bits);
			const auto d = IRDB_SDK::DecodedInstruction_t::factory(ret);
			assert(d->valid());
			return ret;
		}
};
#endif
