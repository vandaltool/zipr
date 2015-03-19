#ifndef _cinderella_prep_h
#define _cinderella_prep_h

#include <libIRDB-core.hpp>
#include <libIRDB-cfg.hpp>

#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"

using namespace libIRDB;

class CinderellaPrep {
	public:
		CinderellaPrep(FileIR_t *p_firp);
		bool execute();

	private:
		Instruction_t* findProgramEntryPoint();
		void addInferenceCallback(Instruction_t *);
		void pinAllFunctionEntryPoints();

	private:
		FileIR_t *m_firp;
		ELFIO::elfio* m_elfiop;
		Callgraph_t m_cg;
};

#endif
