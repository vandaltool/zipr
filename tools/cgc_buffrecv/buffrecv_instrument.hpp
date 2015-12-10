/*
 * Copyright (c) 2015 - University of Virginia
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
 */

#ifndef buffrecv_instrument_hpp
#define buffrecv_instrument_hpp

#include <libIRDB-core.hpp>
#include <libIRDB-util.hpp>
#include <libIRDB-syscall.hpp>

#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"

class BuffRecv_Instrument
{
	public:
		BuffRecv_Instrument(libIRDB::FileIR_t *the_firp) : firp(the_firp), syscalls(firp)
		{
                        int elfoid=firp->GetFile()->GetELFOID();
                        pqxx::largeobject lo(elfoid);
			libIRDB::pqxxDB_t *interface=dynamic_cast<libIRDB::pqxxDB_t*>(libIRDB::BaseObj_t::GetInterface());
			assert(interface);
                        lo.to_file(interface->GetTransaction(),"readeh_tmp_file.exe");

                        elfiop=new ELFIO::elfio;
                        elfiop->load("readeh_tmp_file.exe");
                        ELFIO::dump::header(std::cout,*elfiop);
                        ELFIO::dump::section_headers(std::cout,*elfiop);
                        ELFIO::dump::segment_headers(std::cout,*elfiop);
		} 
		virtual ~BuffRecv_Instrument() { delete elfiop; }
		bool execute();

	private:
		// main tasks
		bool _add_buffered_receive_instrumentation(libIRDB::Instruction_t *site);
		bool add_buffered_receive_instrumentation();
		std::ostream& displayStatistics(std::ostream &os);

	private:
		libIRDB::FileIR_t* firp;
		libIRDB::Syscalls_t syscalls;
		ELFIO::elfio*    elfiop;
};

#endif

