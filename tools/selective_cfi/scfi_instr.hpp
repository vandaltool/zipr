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
#include <memory>



class SCFI_Instrument
{
	public:
		SCFI_Instrument(libIRDB::FileIR_t *the_firp, 
                                int p_nonce_size=1,
                                int p_exe_nonce_size=4,
				bool p_do_coloring=true,
                                bool p_do_color_exe_nonces=true,
				bool p_do_common_slow_path=true,
				bool p_do_jumps=false,
				bool p_do_calls=true,
				bool p_do_rets=true,
				bool p_do_safefn=true,
				bool p_do_multimodule=false,
				bool p_do_exe_nonce_for_call=false
			) 
			: 
		  	  firp(the_firp), 
                          nonce_size(p_nonce_size),
                          exe_nonce_size(p_exe_nonce_size),
			  do_coloring(p_do_coloring), 
                          do_color_exe_nonces(p_do_color_exe_nonces),
			  do_common_slow_path(p_do_common_slow_path), 
			  do_jumps(p_do_jumps), 
			  do_calls(p_do_calls), 
			  do_rets(p_do_rets), 
			  do_multimodule(p_do_multimodule), 
			  protect_safefn(p_do_safefn), 
			  do_exe_nonce_for_call(p_do_exe_nonce_for_call), 
			  ret_shared(NULL),
			  zestcfi_function_entry(NULL),
			  ExecutableNonceValue("\x90", 1)
		{ 
			std::cout<<std::boolalpha;
			std::cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::do_coloring="<<p_do_coloring<<std::endl;
                        std::cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::do_color_exe_nonces="<<p_do_color_exe_nonces<<std::endl;
			std::cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::do_common_slow_path="<<p_do_common_slow_path<<std::endl;
			std::cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::do_jumps="<<p_do_jumps<<std::endl;
			std::cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::do_calls="<<p_do_calls<<std::endl;
			std::cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::do_rets="<<p_do_rets<<std::endl;
			std::cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::do_safefn="<<p_do_safefn<<std::endl;
			std::cout<<"# ATTRIBUTE Selective_Control_Flow_Integrity::do_multimodule="<<p_do_multimodule<<std::endl;
			preds.AddFile(the_firp); 

		}
		bool execute();

	private: // methods

		// helpers for adding GOT entries and symbols for multi-module cfi	
		template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int B>
		bool add_dl_support();

		template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int B>
		bool add_libdl_as_needed_support();

		template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
		bool add_got_entries();

		template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
		libIRDB::Instruction_t* find_runtime_resolve(libIRDB::DataScoop_t* gotplt_scoop);

		template<typename T_Elf_Sym, typename T_Elf_Rela, typename T_Elf_Dyn, int reloc_type, int rela_shift, int ptrsize>
		void add_got_entry(const std::string& name);




		// find instrumentation points.
		bool mark_targets();
		bool instrument_jumps();

		// helper
                libIRDB::Instruction_t* GetExeNonceSlowPath(libIRDB::Instruction_t* insn);
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

		unsigned int GetExeNonceOffset(libIRDB::Instruction_t* insn);
		NonceValueType_t GetExeNonce(libIRDB::Instruction_t* insn);
                unsigned int GetExeNonceSize(libIRDB::Instruction_t* insn);


	private: // data
		// predecessors of instructions.
		libIRDB::InstructionPredecessors_t preds;
		libIRDB::FileIR_t* firp;
                int nonce_size;
                int exe_nonce_size;
		bool do_coloring;
                bool do_color_exe_nonces;
		bool do_common_slow_path;
		bool do_jumps;
		bool do_calls;
		bool do_rets;
		bool do_multimodule;
		bool protect_safefn;
		bool do_exe_nonce_for_call;
		std::unique_ptr<ColoredInstructionNonces_t> color_map;
                std::unique_ptr<ColoredInstructionNonces_t> exe_nonce_color_map;
		libIRDB::Instruction_t *ret_shared;
		libIRDB::Instruction_t *zestcfi_function_entry;
		std::string ExecutableNonceValue;
                
                // Exe Nonce helpers
                int EXE_NONCE_OPCODE_SIZE = 3;
                // Enter opcode val in reverse-byte order, as nonce_relocs reverses the bytes before placement
                // (The nonce values themselves are placed little endian, but the opcode should be placed big-endian)
                int EXE_NONCE_OPCODE_VAL = 0x801F0F; // actual opcode = 0x0F1F80
                int EXE_NONCE_ARG_SIZE = 4;
                /* Nonces are just bit strings. To fit them into exe
                 * nonces that have a limited argument size, they may need to
                 * be split over more than one exe nonce. To be stored as tightly 
                 * as possible, more than one nonce may be fit into one exe nonce. 
                 * 
                 * To handle this, the GetExeNonceFit function returns a list
                 * of one or more NonceParts that together specify where in memory
                 * the complete nonce string is located. */
                struct NoncePart_t {
                    NonceValueType_t val;
                    int bytePos;
                    int size;
                };
                /* CRITICAL ASSUMPTIONS:
                 *   1.) The exe nonce being used is constant throughout the module.
                 *       (It's opcode size, argument size, and opcode value should be constant).
                 *   2.) No nonce is split across exe nonces unless it completely fills all of them.
                 *       (i.e. nonceSize % argSize == 0 OR argSize % nonceSize == 0).  
                 */
                std::vector<NoncePart_t> GetExeNonceFit(NonceValueType_t nonceVal, int nonceSz, int noncePos);
		int GetNonceBytePos(int nonceSz, int noncePos);
                void CreateExeNonceReloc(libIRDB::Instruction_t* insn, NonceValueType_t nonceVal, int nonceSz, int bytePos);
                void PlaceExeNonceReloc(libIRDB::Instruction_t* insn, NonceValueType_t nonceVal, int nonceSz, int noncePos);
                void InsertExeNonceComparisons(libIRDB::Instruction_t* insn, NonceValueType_t nonceVal, int nonceSz, 
                                                              int noncePos, libIRDB::Instruction_t* exeNonceSlowPath );
};

#endif
