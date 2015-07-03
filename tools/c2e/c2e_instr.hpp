/*
 * Copyright (c) 2014, 2015 - University of Virginia 
 *
 * This file may be used and modified for non-commercial purposes as long as 
 * all copyright, permission, and nonwarranty notices are preserved.  
 * Redistribution is prohibited without prior written consent from the University 
 * of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: University of Virginia
 * e-mail: jwd@virginia.com
 * URL   : http://www.cs.virginia.edu/
 *
 */

#ifndef c2e_instrument_hpp
#define c2e_instrument_hpp

#include <libIRDB-core.hpp>

#include <syscall.h>



class Cgc2Elf_Instrument
{
	public:
		Cgc2Elf_Instrument(libIRDB::FileIR_t *the_firp) : firp(the_firp) {
			forceReadFromStdin = false;
			forceExitOnReadEOF = false;
			forceWriteToStdout = false;
			forceWriteFd = -1;
		}
		bool execute();

		void setForceReadFromStdin(bool force) { forceReadFromStdin = force; }
		void setForceExitOnReadEOF(bool force) { forceExitOnReadEOF = force; }
		void setForceWriteToStdout(bool force, int fd = 1) { forceWriteToStdout = force; forceWriteFd = fd; }

		bool getForceReadFromStdin() const { return forceReadFromStdin; }
		bool getForceExitOnReadEOF() const { return forceExitOnReadEOF; }
		bool getForceWriteToStdout() const { return forceWriteToStdout; }
		int getForceWriteFd() const { return forceWriteFd; }

	private:

		libIRDB::Instruction_t* insertTerminate(libIRDB::Instruction_t* after) ;
		libIRDB::Instruction_t* insertTransmit(libIRDB::Instruction_t* after, int sysno=SYS_write, int force_fd=-1);
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

		bool forceReadFromStdin;
		bool forceExitOnReadEOF;
		bool forceWriteToStdout;
		int forceWriteFd;

};

#endif

