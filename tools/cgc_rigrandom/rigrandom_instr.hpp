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

