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

#include <irdb-core>
#include <stdint.h>


typedef uint64_t NonceValueType_t;

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
		ColoredSlotAllocator_t(int sn, int mv) : slot_number(sn), used(1), max_value(mv) { }

		bool CanReserve() const { return used < max_value; }
		ColoredSlotValue_t Reserve() 
		{ 
			assert(CanReserve()); 
			return ColoredSlotValue_t(slot_number, used++); 
		}
		int SlotsUsed() const { return used ; }
 
	
	private:
		int slot_number;
		NonceValueType_t used;
		NonceValueType_t max_value;
};

typedef std::map<int,ColoredSlotValue_t> ColoredSlotValues_t;


class ColoredInstructionNonces_t 
{
	public:
		ColoredInstructionNonces_t(IRDB_SDK::FileIR_t *the_firp)
			: firp(the_firp), slot_size(1), slot_values(255) { }
		ColoredInstructionNonces_t(IRDB_SDK::FileIR_t *the_firp, int the_slot_size)
			: firp(the_firp), slot_size(the_slot_size), 
                          slot_values(MaxNonceValForSlotSize(the_slot_size))  { }               
		ColoredSlotValues_t  GetColorsOfIBT (IRDB_SDK::Instruction_t* i) 
		{ return color_assignments[i]; }

		ColoredSlotValue_t  GetColorOfIB (IRDB_SDK::Instruction_t* i) 
		{ assert(i->getIBTargets()); return slot_assignments[*i->getIBTargets()]; }

		int NumberSlotsUsed() { return slots_used.size(); }

		bool build() { return create(); }

	private:

		// helper to assign colors to slots.
		bool create();

		// the IR we're working on.
		IRDB_SDK::FileIR_t* firp;

		// used to describe how big a nonce slot is.  for now, 1 byte.
		const int slot_size;
		const NonceValueType_t slot_values;

		// information for each slot we've used.
		std::vector<ColoredSlotAllocator_t> slots_used;

		// information for each IBT.
		// a map for each instruction, which contains a ColorSlotValue_t for each slot used.
		//  instruction -> ( int-> slot_value )
		std::map<IRDB_SDK::Instruction_t*, ColoredSlotValues_t> color_assignments;

		// information for each IB (as indexed by the IBs ICFS).
		// the slot that each IB uses. ICFS_t -> slot_value
		std::map<IRDB_SDK::InstructionSet_t, ColoredSlotValue_t> slot_assignments;

                NonceValueType_t MaxNonceValForSlotSize(int slot_size)
                {
                    NonceValueType_t max_nonce_val = ~((NonceValueType_t) 0);
                    size_t max_nonce_size_bits = sizeof(NonceValueType_t)*8;
                    size_t slot_size_bits = slot_size*8;

                    return max_nonce_val >> (max_nonce_size_bits - slot_size_bits);
                }


};


// a simple way to sort ICFS.   

class UniqueICFSSetSorter_t;
typedef std::set<IRDB_SDK::InstructionSet_t, UniqueICFSSetSorter_t> UniqueICFSSet_t;

class UniqueICFSSetSorter_t
{
        public:
                bool operator() (const IRDB_SDK::InstructionSet_t& a, const IRDB_SDK::InstructionSet_t& b) const
                {
                        if(a.size() == b.size()) return a<b;
                        return a.size() < b.size() ;
                }
};

#endif
