#ifndef ARCHARM64_HPP
#define ARCHARM64_HPP

class ZiprArchitectureHelperARM64_t : public ZiprArchitectureHelperBase_t
{
	public:
		virtual libIRDB::Instruction_t* createNewJumpInstruction(libIRDB::FileIR_t *p_firp, libIRDB::Instruction_t* p_existing) override
		{
			const auto bits =string("\x06\x00\x00\x00",4);
			return IRDBUtility::addNewDatabits(p_firp, p_existing, bits);
			// A Brandh unconditional, in binary is: 0001010 00000000 00000000 00000000
			// it includes a 26-bit immediate, which is +/- 128MB, which should be a good enough "jump anywhere"
			// for now.
		}
		virtual libIRDB::Instruction_t* createNewHaltInstruction(libIRDB::FileIR_t *p_firp, libIRDB::Instruction_t* p_existing) override
		{
			const auto bits =string("\xd4\x40\x00\x00",4);
			return IRDBUtility::addNewDatabits(p_firp, p_existing, bits);
			// A halt unconditional, in binary is: 
			// 1101 0100  0100 0000  0000 0000  0000 0000
		}
};
#endif
