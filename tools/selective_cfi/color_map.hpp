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

#ifndef color_map_hpp
#define color_map_hpp

#include <libIRDB-core.hpp>
#include <stdint.h>


typedef int64_t NonceValueType_t;

class ColoredSlotValue_t
{
	public:
		ColoredSlotValue_t(int np, NonceValueType_t nv) : position(np), nonce_value(nv), valid(true) { };
		ColoredSlotValue_t() : valid(false) {}

		// void SetPosition(int pos) { slot_position=pos; }
		int GetPosition() const { return position;}

		// void SetNonceValue(NonceValueType_t p_nv) { nonce_value=p_nv; }
		NonceValueType_t const GetNonceValue() { return nonce_value; }

		bool IsValid() const { return valid; }

	private:
		int position;
		NonceValueType_t nonce_value;
		bool valid;
};

class ColoredSlotAllocator_t
{
	public:
		ColoredSlotAllocator_t(int sn, int mv) : slot_number(sn), used(0), max_value(mv) { }

		bool CanReserve() const { return used < max_value; }
		ColoredSlotValue_t Reserve() 
		{ 
			assert(CanReserve()); 
			return ColoredSlotValue_t(slot_number, used++); 
		}
		int SlotsUsed() const { return used ; }
 
	
	private:
		int slot_number;
		int used;
		int max_value;
};

typedef std::map<int,ColoredSlotValue_t> ColoredSlotValues_t;


class ColoredInstructionNonces_t 
{
	public:
		ColoredInstructionNonces_t(libIRDB::FileIR_t *the_firp)
			: firp(the_firp) { }

		ColoredSlotValues_t  GetColorsOfIBT (libIRDB::Instruction_t* i) 
		{ return color_assignments[i]; }

		ColoredSlotValue_t  GetColorOfIB (libIRDB::Instruction_t* i) 
		{ assert(i->GetIBTargets()); return slot_assignments[*i->GetIBTargets()]; }

		int NumberSlotsUsed() { return slots_used.size(); }

		bool build() { return create(); }

	private:

		// helper to assign colors to slots.
		bool create();

		// the IR we're working on.
		libIRDB::FileIR_t* firp;

		// used to describe how big a nonce slot is.  for now, 1 byte.
		const int slot_size=1;
		const int slot_values=255;

		// information for each slot we've used.
		std::vector<ColoredSlotAllocator_t> slots_used;

		// information for each IBT.
		// a map for each instruction, which contains a ColorSlotValue_t for each slot used.
		//  instruction -> ( int-> slot_value )
		std::map<libIRDB::Instruction_t*, ColoredSlotValues_t> color_assignments;

		// information for each IB (as indexed by the IBs ICFS).
		// the slot that each IB uses. ICFS_t -> slot_value
		std::map<libIRDB::ICFS_t, ColoredSlotValue_t> slot_assignments;


};


// a simple way to sort ICFS.   

class UniqueICFSSetSorter_t;
typedef std::set<libIRDB::ICFS_t, UniqueICFSSetSorter_t> UniqueICFSSet_t;

class UniqueICFSSetSorter_t
{
        public:
                bool operator() (const libIRDB::ICFS_t& a, const libIRDB::ICFS_t& b) const
                {
                        if(a.size() == b.size()) return a<b;
                        return a.size() < b.size() ;
                }
};

#endif

