/*
 * Copyright (c) 2015 - University of Virginia
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from the 
 * University of Virginia.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef hlx_hpp
#define hlx_hpp

#include <libIRDB-core.hpp>

class HLX_Instrument
{
	public:
		HLX_Instrument(libIRDB::FileIR_t *the_firp) : m_firp(the_firp) {};
		virtual ~HLX_Instrument() {}
		bool execute();

	private:
		libIRDB::Function_t* findFunction(std::string);
		bool padSize(libIRDB::Function_t* const);

	private:
		libIRDB::FileIR_t* m_firp;
};

#endif
