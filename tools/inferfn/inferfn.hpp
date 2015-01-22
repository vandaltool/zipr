#ifndef inferfn_h
#define inferfn_h

#include <libIRDB-core.hpp>
#include <libIRDB-cfg.hpp>

#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"

using namespace libIRDB;

class InferFn {
	public:
		InferFn(FileIR_t *p_firp);
		bool execute();

	private:
		Instruction_t* findEntryPoint();
		void addInferenceCallback(Instruction_t *);
		void pinAllFunctionEntryPoints();

	private:
		FileIR_t *m_firp;
		ELFIO::elfio* m_elfiop;
		Callgraph_t m_cg;
};

#endif
