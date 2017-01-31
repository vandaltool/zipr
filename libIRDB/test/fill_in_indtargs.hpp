
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

#include <libIRDB-core.hpp>
#include <iostream>
#include <limits>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <regex.h>
#include <ctype.h>
#include <list>
#include <stdio.h>

#include <exeio.h>
#include "beaengine/BeaEngine.h"
#include "check_thunks.hpp"

using namespace libIRDB;
using namespace std;
using namespace EXEIO;

/*
 * defines 
 */
#define arch_ptr_bytes() (firp->GetArchitectureBitWidth()/8)

/* 
 * global variables 
 */


//
// data structures
//

class ibt_provenance_t
{
	public:
		typedef unsigned int provtype_t;

		ibt_provenance_t() : value(0) { };

		ibt_provenance_t(const provtype_t t) : value(t) { };

		static const provtype_t ibtp_eh_frame=1<<0;
		static const provtype_t ibtp_user=1<<1;	// requested by user
		static const provtype_t ibtp_gotplt=1<<2;
		static const provtype_t ibtp_initarray=1<<3;
		static const provtype_t ibtp_finiarray=1<<4;
		static const provtype_t ibtp_entrypoint=1<<5;
		static const provtype_t ibtp_data=1<<6;
		static const provtype_t ibtp_text=1<<7;
		static const provtype_t ibtp_texttoprintf=1<<8;
		static const provtype_t ibtp_dynsym=1<<9;
		static const provtype_t ibtp_symtab=1<<10;
		static const provtype_t ibtp_stars_ret=1<<11;
		static const provtype_t ibtp_stars_switch=1<<12;
		static const provtype_t ibtp_stars_data=1<<13;
		static const provtype_t ibtp_stars_unknown=1<<14;
		static const provtype_t ibtp_stars_addressed=1<<15;
		static const provtype_t ibtp_stars_unreachable=1<<16;
		static const provtype_t ibtp_switchtable_type1=1<<17;
		static const provtype_t ibtp_switchtable_type2=1<<18;
		static const provtype_t ibtp_switchtable_type3=1<<19;
		static const provtype_t ibtp_switchtable_type4=1<<20;
		static const provtype_t ibtp_switchtable_type5=1<<21;
		static const provtype_t ibtp_switchtable_type6=1<<22;
		static const provtype_t ibtp_switchtable_type7=1<<23;
		static const provtype_t ibtp_switchtable_type8=1<<24;
		static const provtype_t ibtp_switchtable_type9=1<<25;
		static const provtype_t ibtp_switchtable_type10=1<<26;
		static const provtype_t ibtp_rodata=1<<27;
		static const provtype_t ibtp_unknown=1<<28;	// completely unknown
		static const provtype_t ibtp_got=1<<29;	// got is 0 init'd, shouldn't see this one.
		static const provtype_t ibtp_ret=1<<30;	// insn after a call

		void add(const provtype_t t) { value |=t; }
		void add(const ibt_provenance_t t) { value |=t.value; }
		bool isFullySet(const provtype_t t) const { return (value&t) == t; }
		bool isFullySet(const ibt_provenance_t t) const { return (value&t.value) == t.value; }
		bool isPartiallySet(const provtype_t t) const { return (value&t) != 0; }
		bool isPartiallySet(const ibt_provenance_t t) const { return (value&t.value) != 0; }

		bool areOnlyTheseSet(const provtype_t t) const { return (value&~t) == 0; }
		bool areOnlyTheseSet(const ibt_provenance_t t) const { return (value&~t.value) == 0; }
		bool isEmpty() const { return value==0; }

	private:

		provtype_t value;
		
};


/*
 * Forward prototypes 
 */

bool is_possible_target(virtual_offset_t p, virtual_offset_t addr);
bool possible_target(virtual_offset_t p, virtual_offset_t from_addr, ibt_provenance_t prov=ibt_provenance_t::ibtp_unknown);


class fii_icfs : public ICFS_t
{
	public:
		// get/set table start
		virtual_offset_t GetTableStart() {return table_start; }
		void SetTableStart(virtual_offset_t s) {table_start=s; }

		// get/set switch type
		ibt_provenance_t GetSwitchType() { return switch_type; }
		void AddSwitchType(const ibt_provenance_t& p) { switch_type.add(p); }

		// get/set table size
		int GetTableSize() { return table_size; }
		void SetTableSize(int s) { table_size=s; }
	private:

		virtual_offset_t table_start;
		ibt_provenance_t switch_type;
		int table_size;

};

void split_eh_frame(FileIR_t* firp);

