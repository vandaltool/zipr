#ifndef EhUpdater
#define EhUpdater

#include <irdb-core>
#include "PNStackLayout.hpp"

class EhUpdater_t 
{
	public:

	EhUpdater_t(IRDB_SDK::FileIR_t* p_firp, IRDB_SDK::Function_t* p_func, PNStackLayout* p_layout)
		:
		m_firp(p_firp),
		m_func(p_func),
		m_layout(p_layout)
	{
	}

	bool execute();

	private:

	bool update_instructions();
	bool update_instructions(IRDB_SDK::Instruction_t* insn);
	bool update_program(IRDB_SDK::EhProgram_t* ehpgm);


	IRDB_SDK::FileIR_t* m_firp;
	IRDB_SDK::Function_t* m_func; 
	PNStackLayout* m_layout;
};

#endif
