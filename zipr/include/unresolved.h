/***************************************************************************
 * Copyright (c)  2014  Zephyr Software LLC. All rights reserved.
 *
 * This software is furnished under a license and/or other restrictive
 * terms and may be used and copied only in accordance with such terms
 * and the inclusion of the above copyright notice. This software or
 * any other copies thereof may not be provided or otherwise made
 * available to any other person without the express written consent
 * of an authorized representative of Zephyr Software LCC. Title to,
 * ownership of, and all rights in the software is retained by
 * Zephyr Software LCC.
 *
 * Zephyr Software LLC. Proprietary Information
 *
 * Unless otherwise specified, the information contained in this
 * directory, following this legend, and/or referenced herein is
 * Zephyr Software LLC. (Zephyr) Proprietary Information. 
 *
 * CONTACT
 *
 * For technical assistance, contact Zephyr Software LCC. at:
 *      
 *
 * Zephyr Software, LLC
 * 2040 Tremont Rd
 * Charlottesville, VA 22911
 *
 * E-mail: jwd@zephyr-software.com
 **************************************************************************/

#ifndef unresolved_h
#define unresolved_h



enum UnresolvedType_t 
{
	CondJump,
	UncondJump_rel8,
	UncondJump_rel32,
	UncondJump_rel26,
	Call,
	Data32,
	Data64
};

class UnresolvedInfo_t
{
	public:
		virtual IRDB_SDK::Instruction_t* getInstrution() const  =0 ;
	private:
};

// instructions that need a pin, but don't yet have one
class UnresolvedPinned_t : public UnresolvedInfo_t
{
	public:
		UnresolvedPinned_t(IRDB_SDK::Instruction_t* p_from) : from_instruction(p_from), m_range(0,0), m_updated_address(0) { assert(p_from); }
		UnresolvedPinned_t(IRDB_SDK::Instruction_t* p_from, Range_t range) : from_instruction(p_from), m_range(range), m_updated_address(0) { assert(p_from); }
		IRDB_SDK::Instruction_t* getInstrution() const { return from_instruction; }

		/*
		 * Use the range to store the place where 
		 * reserved space is held for this
		 * instruction.
		 */
		Range_t GetRange() const { return m_range; };
		void SetRange(Range_t range) { m_range = range; };
		bool HasRange()
		{
			return m_range.getStart() != 0 || m_range.getEnd() != 0;
		};

		/*
		 * Store an address with the UnresolvedPinned
		 * in case this instruction needs to float
		 * through the program based on a chain
		 * of two-byte calls.
		 */
		bool HasUpdatedAddress() const
		{
			return m_updated_address != 0;
		};
		void SetUpdatedAddress(RangeAddress_t address)
		{
			m_updated_address = address;
		};
		RangeAddress_t GetUpdatedAddress() const
		{
			return m_updated_address;
		};

	private:
		IRDB_SDK::Instruction_t* from_instruction;
		Range_t m_range;
		RangeAddress_t m_updated_address;

	friend bool operator< (const UnresolvedPinned_t& lhs, const UnresolvedPinned_t& rhs);
		
};

inline bool operator< (const UnresolvedPinned_t& lhs, const UnresolvedPinned_t& rhs)
{ 
	assert(lhs.from_instruction);
	assert(rhs.from_instruction);
	return lhs.from_instruction < rhs.from_instruction; 
}

// an ELF location that needs patching when an Unresolved instrcution
class Patch_t
{
	public:
		Patch_t(RangeAddress_t p_from_addr, UnresolvedType_t p_t) : from_addr(p_from_addr), type(p_t) {}

		RangeAddress_t getAddress() const { return from_addr; }
		UnresolvedType_t getType() { return type; }
		void setType(UnresolvedType_t p_t) { type = p_t; }
		size_t getSize() { 
			switch (type) {
				case UncondJump_rel8:
					return 2;
				case UncondJump_rel32:
					return 5;
				default:
					return 0;
			}
		}

	private:
		RangeAddress_t from_addr;
		UnresolvedType_t type;
};

// instructions that can float to any address, but don't yet have one
class UnresolvedUnpinned_t  : public UnresolvedInfo_t
{
	public:
		UnresolvedUnpinned_t(UnresolvedPinned_t up) : from_instruction(up.getInstrution()) {}
		UnresolvedUnpinned_t(IRDB_SDK::Instruction_t* p_from) : from_instruction(p_from) 
		{ assert(p_from); }
		IRDB_SDK::Instruction_t* getInstrution() const { assert(from_instruction); return from_instruction; }
	private:
		IRDB_SDK::Instruction_t *from_instruction;
		
	friend bool operator< (const UnresolvedUnpinned_t& lhs, const UnresolvedUnpinned_t& rhs);
};

inline bool operator< (const UnresolvedUnpinned_t& lhs, const UnresolvedUnpinned_t& rhs)
{ 
	assert(lhs.from_instruction);
	assert(rhs.from_instruction);
	return lhs.from_instruction < rhs.from_instruction; 
}

typedef std::pair<UnresolvedUnpinned_t, Patch_t> UnresolvedUnpinnedPatch_t;
#endif
