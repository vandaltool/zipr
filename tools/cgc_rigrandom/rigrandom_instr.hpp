#ifndef rigrandom_instrument_hpp
#define rigrandom_instrument_hpp

#include <libIRDB-core.hpp>

#include <syscall.h>



class RigRandom_Instrument
{
	public:
		RigRandom_Instrument(libIRDB::FileIR_t *the_firp, char random_start='A') : firp(the_firp), random_start(random_start) {}
		bool execute();

	private:

		libIRDB::Instruction_t* insertRandom(libIRDB::Instruction_t* after) ;

		bool add_rr_instrumentation(libIRDB::Instruction_t* insn);
		bool needs_rr_instrumentation(libIRDB::Instruction_t* insn);

		bool instrument_ints();
	
		libIRDB::FileIR_t* firp;
		char random_start;
};

#endif

