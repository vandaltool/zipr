
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

#include <irdb-core>
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
#include "check_thunks.hpp"


/*
 * defines 
 */
#define arch_ptr_bytes() (firp->getArchitectureBitWidth()/8u)

/* 
 * global variables 
 */


//
// data structures
//


class ibt_provenance_t; 
static inline std::ostream& operator<<(std::ostream& out, const ibt_provenance_t& prov);

class ibt_provenance_t
{
	public:
		typedef uint32_t provtype_t;

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
		friend std::ostream& operator<<(std::ostream& out, const ibt_provenance_t& prov);
		
};

static inline std::ostream& operator<<(std::ostream& out, const ibt_provenance_t& prov)
{
#define print_prov(a) 						\
{						 		\
	if(prov.value&(ibt_provenance_t::a))			\
	{ 							\
		const auto foo=std::string(#a);			\
		out<<foo.substr(4,foo.length())<<",";		\
	}							\
}		
	print_prov(ibtp_eh_frame);
	print_prov(ibtp_user);
	print_prov(ibtp_gotplt);
	print_prov(ibtp_initarray);
	print_prov(ibtp_finiarray);
	print_prov(ibtp_entrypoint);
	print_prov(ibtp_data);
	print_prov(ibtp_text);
	print_prov(ibtp_texttoprintf);
	print_prov(ibtp_dynsym);
	print_prov(ibtp_symtab);
	print_prov(ibtp_stars_ret);
	print_prov(ibtp_stars_switch);
	print_prov(ibtp_stars_data);
	print_prov(ibtp_stars_unknown);
	print_prov(ibtp_stars_addressed);
	print_prov(ibtp_stars_unreachable);
	print_prov(ibtp_switchtable_type1);
	print_prov(ibtp_switchtable_type2);
	print_prov(ibtp_switchtable_type3);
	print_prov(ibtp_switchtable_type4);
	print_prov(ibtp_switchtable_type5);
	print_prov(ibtp_switchtable_type6);
	print_prov(ibtp_switchtable_type7);
	print_prov(ibtp_switchtable_type8);
	print_prov(ibtp_switchtable_type9);
	print_prov(ibtp_switchtable_type10);
	print_prov(ibtp_rodata);
	print_prov(ibtp_unknown);
	print_prov(ibtp_got);
	print_prov(ibtp_ret);
#undef print_prov

	return out;
}


/*
 * Forward prototypes 
 */

bool is_possible_target(IRDB_SDK::VirtualOffset_t p, IRDB_SDK::VirtualOffset_t addr);
bool possible_target(IRDB_SDK::VirtualOffset_t p, IRDB_SDK::VirtualOffset_t from_addr, ibt_provenance_t prov=ibt_provenance_t::ibtp_unknown);


class fii_icfs  : public IRDB_SDK::InstructionSet_t
{
	public:
		// get/set table start
		IRDB_SDK::VirtualOffset_t GetTableStart() {return table_start; }
		void SetTableStart(IRDB_SDK::VirtualOffset_t s) {table_start=s; }

		// get/set switch type
		ibt_provenance_t GetSwitchType() { return switch_type; }
		void AddSwitchType(const ibt_provenance_t& p) { switch_type.add(p); }

		// get/set table size
		int GetTableSize() { return table_size; }
		void SetTableSize(int s) { table_size=s; }

                void addTargets(const IRDB_SDK::InstructionSet_t &other)
                {
                        insert(std::begin(other), std::end(other));
                }
		bool isIncomplete() const {
                        return getAnalysisStatus() == IRDB_SDK::iasAnalysisIncomplete;
                }

                bool isComplete() const {
                        return getAnalysisStatus() == IRDB_SDK::iasAnalysisComplete;
                }

                bool isModuleComplete() const {
                        return getAnalysisStatus() == IRDB_SDK::iasAnalysisModuleComplete;
                }

                void setAnalysisStatus(const IRDB_SDK::ICFSAnalysisStatus_t p_status) {
                        m_icfs_analysis_status = p_status;
                }

		IRDB_SDK::ICFSAnalysisStatus_t getAnalysisStatus() const {
                        return m_icfs_analysis_status;
                }


	private:

		IRDB_SDK::VirtualOffset_t table_start;
		ibt_provenance_t switch_type;
		int table_size;
		IRDB_SDK::ICFSAnalysisStatus_t m_icfs_analysis_status;

};

void split_eh_frame(IRDB_SDK::FileIR_t* firp);

