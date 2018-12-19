#ifndef ARCHX86_HPP
#define ARCHX86_HPP


class ZiprArchitectureHelperX86_t  : public ZiprArchitectureHelperBase_t
{
	public:
		virtual libIRDB::Instruction_t* createNewJumpInstruction(libIRDB::FileIR_t *p_firp, libIRDB::Instruction_t* p_existing) override
		{
			return IRDBUtility::addNewAssembly(p_firp, p_existing, "jmp 0");
		}
};
#endif
