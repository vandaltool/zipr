/*
 * Copyright (c) 2014 - Zephyr Software
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

#ifndef rss_instrument_hpp
#define rss_instrument_hpp

#include <libIRDB-core.hpp>
#include <libIRDB-util.hpp>
#include <libIRDB-syscall.hpp>

#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"




class WSC_Instrument
{
	public:
		WSC_Instrument(libIRDB::FileIR_t *the_firp) : firp(the_firp), syscalls(firp) 
		{
			last_startup_insn=NULL;
                        int elfoid=firp->GetFile()->GetELFOID();
                        pqxx::largeobject lo(elfoid);
			libIRDB::pqxxDB_t *interface=dynamic_cast<libIRDB::pqxxDB_t*>(libIRDB::BaseObj_t::GetInterface());
			assert(interface);
                        lo.to_file(interface->GetTransaction(),"readeh_tmp_file.exe");

			// start off by saying we'll protect all instructions
			to_protect=firp->GetInstructions();

                        elfiop=new ELFIO::elfio;
                        elfiop->load("readeh_tmp_file.exe");
                        ELFIO::dump::header(std::cout,*elfiop);
                        ELFIO::dump::section_headers(std::cout,*elfiop);
                        ELFIO::dump::segment_headers(std::cout,*elfiop);


		} 
		virtual ~WSC_Instrument() { delete elfiop; }
		bool execute();
		bool FindInstructionsToProtect(std::string s);

	private:

		bool add_wsc_instrumentation(libIRDB::Instruction_t *site);
		bool add_init_call();
		bool add_segfault_checking();
		bool add_segfault_checking(libIRDB::Instruction_t*);
		bool add_allocation_instrumentation();
		bool needs_wsc_segfault_checking(libIRDB::Instruction_t*, const DISASM&);
		bool add_receive_limit(libIRDB::Instruction_t* site);
		bool add_receive_limit();

		libIRDB::Instruction_t* GetCallbackCode();
		libIRDB::Instruction_t* FindInstruction(libIRDB::virtual_offset_t addr);
		libIRDB::Relocation_t* create_reloc(libIRDB::Instruction_t* insn, std::string type, int offset);


	private:
		libIRDB::FileIR_t* firp;
		libIRDB::Syscalls_t syscalls;
                ELFIO::elfio*    elfiop;
		libIRDB::Instruction_t *last_startup_insn;

		libIRDB::InstructionSet_t to_protect;
};

#endif

