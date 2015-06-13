#ifndef c2e_instrument_hpp
#define c2e_instrument_hpp

#include <libIRDB-core.hpp>

#include <syscall.h>



class Cgc2Elf_Instrument
{
	public:
		Cgc2Elf_Instrument(libIRDB::FileIR_t *the_firp) : firp(the_firp) {}
		bool execute();

	private:

		libIRDB::Instruction_t* insertTerminate(libIRDB::Instruction_t* after) ;
		libIRDB::Instruction_t* insertTransmit(libIRDB::Instruction_t* after, int sysno=SYS_write);
		libIRDB::Instruction_t* insertReadExitOnEOF(libIRDB::Instruction_t* after);
		libIRDB::Instruction_t* insertReceive(libIRDB::Instruction_t* after, bool force_stdin=true, bool forceExitOnEOF=true) ;
		libIRDB::Instruction_t* insertFdwait(libIRDB::Instruction_t* after) ;
		libIRDB::Instruction_t* insertAllocate(libIRDB::Instruction_t* after) ;
		libIRDB::Instruction_t* insertDeallocate(libIRDB::Instruction_t* after) ;
		libIRDB::Instruction_t* insertRandom(libIRDB::Instruction_t* after) ;



		bool add_c2e_instrumentation(libIRDB::Instruction_t* insn);
		bool needs_c2e_instrumentation(libIRDB::Instruction_t* insn);

		bool instrument_ints();
	
		libIRDB::FileIR_t* firp;


};

#endif

