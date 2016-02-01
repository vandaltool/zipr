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

#ifndef scfi_instrument_hpp
#define scfi_instrument_hpp

#include <libIRDB-core.hpp>
#include "color_map.hpp"



class SCFI_Instrument
{
	public:
		SCFI_Instrument(libIRDB::FileIR_t *the_firp, 
				bool p_do_coloring=true,
				bool p_do_common_slow_path=true,
				bool p_do_jumps=true,
				bool p_do_rets=true) 
			: firp(the_firp), 
			  do_coloring(p_do_coloring), 
			  do_common_slow_path(p_do_common_slow_path), 
			  do_jumps(p_do_jumps), 
			  do_rets(p_do_rets), 
			  color_map(NULL) {}
		bool execute();

	private:


		// find instrumentation points.
		bool mark_targets();
		bool instrument_jumps();

		// helper
		libIRDB::Relocation_t* create_reloc(libIRDB::Instruction_t* insn);
		libIRDB::Relocation_t* FindRelocation(libIRDB::Instruction_t* insn, std::string type);

		// add instrumentation
		bool add_scfi_instrumentation(libIRDB::Instruction_t* insn);
		bool needs_scfi_instrumentation(libIRDB::Instruction_t* insn);

		// return instrumentation
		void  AddReturnCFI(libIRDB::Instruction_t* insn, ColoredSlotValue_t *v=NULL);
		// jump instrumentation
		void AddJumpCFI(libIRDB::Instruction_t* insn);


		// Nonce Manipulation.
		NonceValueType_t GetNonce(libIRDB::Instruction_t* insn);
		unsigned int GetNonceSize(libIRDB::Instruction_t* insn);
		unsigned int GetNonceOffset(libIRDB::Instruction_t*);



	
		libIRDB::FileIR_t* firp;
		bool do_coloring;
		bool do_common_slow_path;
		bool do_jumps;
		bool do_rets;
		ColoredInstructionNonces_t *color_map;


};

#endif

