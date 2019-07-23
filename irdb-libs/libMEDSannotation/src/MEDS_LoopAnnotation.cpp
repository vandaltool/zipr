/*
 * Copyright (c) 2014 - Zephyr Software LLC
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

#include <stdlib.h>

#include <iostream>
#include <cstdio>
#include <string>
#include <string.h>

#include "MEDS_LoopAnnotation.hpp"

using namespace std;
using namespace MEDS_Annotation;





MEDS_LoopAnnotation::MEDS_LoopAnnotation(const string &p_rawLine)
{
	init();
	m_rawInputLine=p_rawLine;
	parse();
}

void MEDS_LoopAnnotation::init()
{
}


template<typename int_type>
static int_type readInt(const string& line, const string& name, const bool is_hex)
{
	const auto pos   = line.find(name);
	assert(pos != string::npos);
	const auto in    = line.substr(pos+name.length());
	auto addr_string = string();
	auto ret         = int_type();

	stringstream iss;
	if(is_hex)
		iss << hex;
	iss<<in;
	iss >> ret;

	if(iss.fail())
		assert(0);

	return ret;
}

template<typename int_type>
static set<int_type> readAddrSet(const string& line, const string& name, const bool is_hex)
{
	const auto pos   = line.find(name);
	assert(pos != string::npos);
	const auto in    = line.substr(pos+name.length());
	auto addr_string = string();
	auto ret         = set<int_type>();

        stringstream ss(in);	 // cannot use auto here.

        while ( ss>>addr_string )
        {
                if( addr_string=="ZZ")
                        return ret;

		auto addr = IRDB_SDK::VirtualOffset_t();
		stringstream iss; // cannot use auto here
		if(is_hex)
			iss << hex;
		iss << addr_string; 	
		iss >> addr;
		assert(!iss.fail());

                ret.insert(addr);
        }

        assert(0);
        abort();        // needed for avoiding errors
}


/*
	Example format (as of July 2019 ) -- subject to change
5f 37 LOOP 0 FIRSTINST 80575fe PREHEADER ffffffffffffffff BLOCKLIST 80575fe 805760c 8057621 ZZ INNERLOOPS ZZ
5f 37 LOOP 1 FIRSTINST 8057637 PREHEADER 8057632          BLOCKLIST 8057637 8057645 805765a ZZ INNERLOOPS ZZ
5f 37 LOOP 2 FIRSTINST 80575ce PREHEADER 80575c7          BLOCKLIST 80575ce 80575de 80575e4 80575f2 8057626 8057632 805765f 80575fe 805760a 805760c 8057618 8057621 8057637 8057643 8057645 8057651 805765a 8057664 ZZ INNERLOOPS 0 1 ZZ

*/
void MEDS_LoopAnnotation::parse()
{

	const auto loop_pos = m_rawInputLine.find(" LOOP ");
        if ( loop_pos == string::npos )
	{
		/* look for loop annotations only */
		return;
	}

        // get offset
        m_virtualOffset = VirtualOffset(m_rawInputLine);


	loop_no    = readInt<decltype(loop_no  )>(m_rawInputLine, " LOOP ",      false);
	header     = readInt<decltype(header   )>(m_rawInputLine, " FIRSTINST ", true);
	preheader  = readInt<decltype(preheader)>(m_rawInputLine, " PREHEADER ", true);

	all_blocks = readAddrSet<uint64_t>(m_rawInputLine, " BLOCKLIST ", true);
	sub_loops  = readAddrSet<uint64_t>(m_rawInputLine, " INNERLOOPS ", false);

	setValid();	// no additional info recorded for right now.
}


