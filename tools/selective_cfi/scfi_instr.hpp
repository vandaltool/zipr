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
#include <libIRDB-util.hpp>
#include "color_map.hpp"
#include <iostream>
#include <iomanip>



class SCFI_Instrument
{
	public:
		SCFI_Instrument(libIRDB::FileIR_t *the_firp, 
				bool p_do_coloring=true,
				bool p_do_common_slow_path=true,
				bool p_do_jumps=false,
				bool p_do_calls=true,
				bool p_do_rets=true,
				bool p_do_safefn=true,
				bool p_do_multimodule=false,
				bool p_do_exe_nonce_for_call=false
			) 
			: firp(the_firp), 
			  do_coloring(p_do_coloring), 
			  do_common_slow_path(p_do_common_slow_path), 
			  do_jumps(p_do_jumps), 
			  do_calls(p_do_calls), 
			  do_rets(p_do_rets), 
			  protect_safefn(p_do_safefn), 
			  do_multimodule(p_do_multimodule), 
			  do_exe_nonce_for_call(p_do_exe_nonce_for_call), 
			  color_map(NULL),
			  ExecutableNonceValue("\x90", 1),
			  ret_shared(NULL)

		{ 
			std::cout<<std::boolalpha;
			std::cout<<"#ATTRIBUTE do_coloring="<<p_do_coloring<<std::endl;
			std::cout<<"#ATTRIBUTE do_common_slow_path="<<p_do_common_slow_path<<std::endl;
			std::cout<<"#ATTRIBUTE do_jumps="<<p_do_jumps<<std::endl;
			std::cout<<"#ATTRIBUTE do_calls="<<p_do_calls<<std::endl;
			std::cout<<"#ATTRIBUTE do_rets="<<p_do_rets<<std::endl;
			std::cout<<"#ATTRIBUTE do_safefn="<<p_do_safefn<<std::endl;
			std::cout<<"#ATTRIBUTE do_multimodule="<<p_do_multimodule<<std::endl;
			preds.AddFile(the_firp); 

			if(do_exe_nonce_for_call && p_do_coloring)
			{
				std::cerr<<"Cannot do coloring+exe_nonce_for_call!"<<std::endl;
				exit(1);
			}
		}
		bool execute();

	private:

		// helpers for adding GOT entries and symbols for multi-module cfi	
		template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int B>
		bool add_dl_support();

		template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int B>
		bool add_libdl_as_needed_support();

		template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
		bool add_got_entries();

		template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
		libIRDB::Instruction_t* find_runtime_resolve(libIRDB::DataScoop_t* gotplt_scoop);



		// find instrumentation points.
		bool mark_targets();
		bool instrument_jumps();

		// helper
		libIRDB::Relocation_t* create_reloc(libIRDB::Instruction_t* insn);
		libIRDB::Relocation_t* FindRelocation(libIRDB::Instruction_t* insn, std::string type);
		bool isSafeFunction(libIRDB::Instruction_t* insn);
		bool isCallToSafeFunction(libIRDB::Instruction_t* insn);
		bool is_jmp_a_fixed_call(libIRDB::Instruction_t* insn);
		bool is_plt_style_jmp(libIRDB::Instruction_t* insn);



		// add instrumentation
		bool add_scfi_instrumentation(libIRDB::Instruction_t* insn);
		bool needs_scfi_instrumentation(libIRDB::Instruction_t* insn);

		// return instrumentation
		void  AddReturnCFI(libIRDB::Instruction_t* insn, ColoredSlotValue_t *v=NULL);
		void  AddReturnCFIForExeNonce(libIRDB::Instruction_t* insn, ColoredSlotValue_t *v=NULL);

		// jump instrumentation
		void AddJumpCFI(libIRDB::Instruction_t* insn);

		// call instrumentation with executable nonce
		void AddCallCFIWithExeNonce(libIRDB::Instruction_t* insn);

		// for all calls
		void AddExecutableNonce(libIRDB::Instruction_t* insn);

		// Nonce Manipulation.
		NonceValueType_t GetNonce(libIRDB::Instruction_t* insn);
		unsigned int GetNonceSize(libIRDB::Instruction_t* insn);
		unsigned int GetNonceOffset(libIRDB::Instruction_t*);


		// predecessors of instructions.
		libIRDB::InstructionPredecessors_t preds;
	
		libIRDB::FileIR_t* firp;
		bool do_coloring;
		bool do_common_slow_path;
		bool do_jumps;
		bool do_calls;
		bool do_rets;
		bool do_multimodule;
		bool protect_safefn;
		bool do_exe_nonce_for_call;

		ColoredInstructionNonces_t *color_map;

		libIRDB::Instruction_t *ret_shared;

		std::string ExecutableNonceValue;
};

#endif

