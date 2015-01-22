#ifndef scfi_instrument_hpp
#define scfi_instrument_hpp

#include <libIRDB-core.hpp>



class SCFI_Instrument
{
	public:
		SCFI_Instrument(libIRDB::FileIR_t *the_firp) : firp(the_firp) {}
		bool execute();

	private:


		// find instrumentation points.
		bool mark_targets();
		bool instrument_jumps();

		// helper
		libIRDB::Relocation_t* create_reloc(libIRDB::Instruction_t* insn);

		// add instrumentation
		bool add_scfi_instrumentation(libIRDB::Instruction_t* insn);
		bool needs_scfi_instrumentation(libIRDB::Instruction_t* insn);

		// return instrumentation
		void  AddReturnCFI(libIRDB::Instruction_t* insn);


		// Nonce Manipulation.
		unsigned int GetNonce(libIRDB::Instruction_t* insn);
		unsigned int GetNonceSize(libIRDB::Instruction_t* insn);

	
		libIRDB::FileIR_t* firp;


};

#endif

