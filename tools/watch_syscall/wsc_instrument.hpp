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

#include "csowarn.hpp"

typedef std::set<CSO_WarningRecord_t*>  CSO_WarningRecordSet_t;
typedef std::map< libIRDB::Instruction_t*, CSO_WarningRecordSet_t> CSO_WarningRecordMap_t;

class WSC_Instrument
{
	public:
		WSC_Instrument(libIRDB::FileIR_t *the_firp) : firp(the_firp), syscalls(firp), fail_code(NULL)
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

			SetSandboxing(false);
			SetReverseSandboxing(false);
			SetPromiscuousSandboxing(false);
			SetInputFiltering(false);
			SetSmartMode(false);

			m_num_segfault_instrumentations = 0;
			m_num_nullcheck_instrumentations = 0;
			m_num_boundscheck_instrumentations = 0;
			m_num_segfault_checking = 0;
		} 
		virtual ~WSC_Instrument() { delete elfiop; }
		bool execute();
		bool FindInstructionsToProtect(std::set<std::string> s, int& num_instructions);

		void SetSandboxing(bool doit) { m_doSandboxing = doit; }
		bool DoSandboxing() const { return m_doSandboxing; }
		void SetReverseSandboxing(bool doit) { m_doReverseSandboxing = doit; }
		bool DoReverseSandboxing() const { return m_doReverseSandboxing; }
		void SetPromiscuousSandboxing(bool doit) { m_doPromiscuousSandboxing = doit; }
		bool DoPromiscuousSandboxing() const { return m_doPromiscuousSandboxing; }
		void SetInputFiltering(bool doit) { m_doInputFiltering = doit; }
		bool DoInputFiltering() const { return m_doInputFiltering; }
		void SetSmartMode(bool doit) { m_doSmart = doit; }
		bool DoSmartMode() const { return m_doSmart; }
		std::ostream& displayStatistics(std::ostream&);

	private:

		// main tasks
		bool add_init_call();
		bool add_wsc_dealloc_instrumentation(libIRDB::Instruction_t *site);
		bool add_wsc_alloc_instrumentation(libIRDB::Instruction_t *site);
		bool add_segfault_checking();
		bool add_smart_checking(libIRDB::Instruction_t*);
		bool add_segfault_checking(libIRDB::Instruction_t*);
		bool add_segfault_checking(libIRDB::Instruction_t* insn, const CSO_WarningRecord_t *const wr);
		bool add_allocation_instrumentation();
		bool needs_wsc_segfault_checking(libIRDB::Instruction_t*, const DISASM&);
		bool needs_reverse_sandboxing(libIRDB::Instruction_t*, const DISASM&);
		bool add_receive_limit(libIRDB::Instruction_t* site);
		bool add_receive_limit();
		bool add_bounds_check(libIRDB::Instruction_t* insn, const CSO_WarningRecord_t *const wr);
		bool add_clamp(libIRDB::Instruction_t* insn, const CSO_WarningRecord_t *const wr);
		bool add_null_check(libIRDB::Instruction_t* insn, const CSO_WarningRecord_t *const wr);

		// helpers.
		libIRDB::Instruction_t* GetCallbackCode();
		libIRDB::Instruction_t* FindInstruction(libIRDB::virtual_offset_t addr);
		libIRDB::Relocation_t* create_reloc(libIRDB::Instruction_t* insn, std::string type, int offset);
		libIRDB::Instruction_t* GetFailCode();


	private:
		libIRDB::FileIR_t* firp;
		libIRDB::Syscalls_t syscalls;
		ELFIO::elfio*    elfiop;
		libIRDB::Instruction_t *last_startup_insn;

		libIRDB::InstructionSet_t to_protect;
		libIRDB::Instruction_t* fail_code;

		CSO_WarningRecordMap_t warning_records;
		bool m_doSandboxing;
		bool m_doReverseSandboxing;
		bool m_doInputFiltering;
		bool m_doPromiscuousSandboxing;
		bool m_doSmart;

		// statistics
		int m_num_segfault_instrumentations;
		int m_num_nullcheck_instrumentations;
		int m_num_boundscheck_instrumentations;
		int m_num_segfault_checking;
};

#endif

