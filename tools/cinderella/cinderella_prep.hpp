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

#ifndef _cinderella_prep_h
#define _cinderella_prep_h

#include <libIRDB-core.hpp>
#include <libIRDB-cfg.hpp>

#include "elfio/elfio.hpp"
#include "elfio/elfio_dump.hpp"

using namespace libIRDB;

class CinderellaPrep {
	public:
		CinderellaPrep(FileIR_t *p_firp);
		bool execute();

	private:
		Instruction_t* findProgramEntryPoint();
		void addInferenceCallback(Instruction_t *);
		void pinAllFunctionEntryPoints();

	private:
		FileIR_t *m_firp;
		ELFIO::elfio* m_elfiop;
		Callgraph_t m_cg;
};

#endif
