/*
 * Copyright (c) 2014-2015 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */

#ifndef scdi_instrument_hpp
#define scdi_instrument_hpp

#include <iostream>
#include <libIRDB-core.hpp>


class SimpleCDI_Instrument
{
	public:
		SimpleCDI_Instrument(libIRDB::FileIR_t *the_firp) : firp(the_firp), 
			single_target_set_jumps(0),
			single_target_set_returns(0),
			num_complete_ibts(0),
			num_returns(0),
			num_complete_returns(0) {}
		bool execute();

	private:


		bool add_scdi_instrumentation(libIRDB::Instruction_t* insn);
		bool needs_scdi_instrumentation(libIRDB::Instruction_t* insn);
		bool convert_ibs();
		void display_stats(std::ostream &out);
	
		libIRDB::FileIR_t* firp;

		int single_target_set_jumps;
		int single_target_set_returns;
		int num_complete_ibts;
		int num_returns;
		int num_complete_returns;
};

#endif

