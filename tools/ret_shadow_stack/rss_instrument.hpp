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

#include "MEDS_AnnotationParser.hpp"
#include <libIRDB-core.hpp>
#include <getopt.h>


class RSS_Instrument
{
	public:
		RSS_Instrument(libIRDB::FileIR_t *the_firp, MEDS_Annotation::MEDS_AnnotationParser* the_meds_ap, bool p_do_zipr) : 
			firp(the_firp), meds_ap(the_meds_ap), do_zipr(p_do_zipr) { };
		bool execute();

		virtual ~RSS_Instrument() {}

	private:
		bool add_rss_push(libIRDB::FileIR_t* firp, libIRDB::Instruction_t* insn);
		bool add_rss_pop(libIRDB::FileIR_t* firp, libIRDB::Instruction_t* insn);
		bool add_rss_instrumentation(libIRDB::FileIR_t* firp, libIRDB::Function_t* func, MEDS_Annotation::MEDS_AnnotationParser *meds_ap);



		libIRDB::FileIR_t* firp;
		MEDS_Annotation::MEDS_AnnotationParser* meds_ap;
		bool do_zipr;
};

#endif

