#ifndef scdi_instrument_hpp
#define scdi_instrument_hpp

#include <libIRDB-core.hpp>



class SimpleCDI_Instrument
{
	public:
		SimpleCDI_Instrument(libIRDB::FileIR_t *the_firp) : firp(the_firp) {}
		bool execute();

	private:


		bool add_scdi_instrumentation(libIRDB::Instruction_t* insn);
		bool needs_scdi_instrumentation(libIRDB::Instruction_t* insn);
		bool convert_ibs();
	
		libIRDB::FileIR_t* firp;


};

#endif

