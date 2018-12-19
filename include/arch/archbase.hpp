#ifndef ARCHBASE_HPP
#define ARCHBASE_HPP


class ZiprArchitectureHelperBase_t
{
	public:
		virtual libIRDB::Instruction_t* createNewJumpInstruction(libIRDB::FileIR_t *p_firp, libIRDB::Instruction_t* p_existing)=0;
		static std::unique_ptr<ZiprArchitectureHelperBase_t> factory(libIRDB::FileIR_t* p_firp);

};

#endif
