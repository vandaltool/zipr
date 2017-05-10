#ifndef EhUpdater
#define EhUpdater

#include <libIRDB-core.hpp>
#include "PNStackLayout.hpp"

class EhUpdater_t 
{
	public:

	EhUpdater_t(libIRDB::FileIR_t* p_firp, libIRDB::Function_t* p_func, PNStackLayout* p_layout)
		:
		m_firp(p_firp),
		m_func(p_func),
		m_layout(p_layout)
	{
	}

	bool execute();

	private:

	bool update_instructions();
	bool update_instructions(libIRDB::Instruction_t* insn);
	bool update_program(libIRDB::EhProgram_t* ehpgm);


	libIRDB::FileIR_t* m_firp;
	libIRDB::Function_t* m_func; 
	PNStackLayout* m_layout;
};

#endif
