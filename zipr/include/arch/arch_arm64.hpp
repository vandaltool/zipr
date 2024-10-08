#ifndef ARCHARM64_HPP
#define ARCHARM64_HPP

class ZiprArchitectureHelperARM64_t : public ZiprArchitectureHelperBase_t
{
	public:
		ZiprArchitectureHelperARM64_t(Zipr_SDK::Zipr_t* p_zipr_obj) : ZiprArchitectureHelperBase_t(p_zipr_obj)
		{
		}
		virtual IRDB_SDK::Instruction_t* createNewJumpInstruction(IRDB_SDK::FileIR_t *p_firp, IRDB_SDK::Instruction_t* p_existing) override
		{
			// A Brandh unconditional, in binary is: 0001 0100  00000000  00000000  00000000
			// it includes a 26-bit immediate, which is +/- 128MB, which should be a good enough "jump anywhere"
			// for now.
			const auto bits =string("\x00\x00\x00\x014",4);
			auto ret=IRDB_SDK::addNewDataBits(p_firp, p_existing, bits);
			const auto d=IRDB_SDK::DecodedInstruction_t::factory(ret);
			assert(d->valid());
			return ret;
		}
		virtual IRDB_SDK::Instruction_t* createNewHaltInstruction(IRDB_SDK::FileIR_t *p_firp, IRDB_SDK::Instruction_t* p_existing) override
		{
			// A halt unconditional, in binary is: 
			// 1101 0100  0100 0000  0000 0000  0000 0000
			// 0xd4400000
			const auto bits =string("\x00\x00\x40\xd4",4);
			auto ret=IRDB_SDK::addNewDataBits(p_firp, p_existing, bits);
			const auto d=IRDB_SDK::DecodedInstruction_t::factory(ret);
			assert(d->valid());
			return ret;
		}
};
#endif
