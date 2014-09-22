/***************************************************************************
 * Copyright (c)  2014  Zephyr Software LLC. All rights reserved.
 *
 * This software is furnished under a license and/or other restrictive
 * terms and may be used and copied only in accordance with such terms
 * and the inclusion of the above copyright notice. This software or
 * any other copies thereof may not be provided or otherwise made
 * available to any other person without the express written consent
 * of an authorized representative of GrammaTech, Inc. Title to,
 * ownership of, and all rights in the software is retained by
 * GrammaTech, Inc.
 *
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
	Call,
	Data32,
	Data64
};

class UnresolvedInfo_t
{
	public:
		virtual libIRDB::Instruction_t* GetInstruction() const  =0 ;
	private:
};



// instructions that can float to any address, but don't yet have one
class UnresolvedUnpinned_t  : public UnresolvedInfo_t
{
	public:
		UnresolvedUnpinned_t(libIRDB::Instruction_t* p_from) : from_instruction(p_from) {}
		libIRDB::Instruction_t* GetInstruction() const { return from_instruction; }
	private:
		libIRDB::Instruction_t *from_instruction;
		
	friend bool operator< (const UnresolvedUnpinned_t& lhs, const UnresolvedUnpinned_t& rhs);
};

inline bool operator< (const UnresolvedUnpinned_t& lhs, const UnresolvedUnpinned_t& rhs)
{ return lhs.from_instruction->GetBaseID() < rhs.from_instruction->GetBaseID(); }

// instructions that need a pin, but don't yet have one
class UnresolvedPinned_t : public UnresolvedInfo_t
{
	public:
		UnresolvedPinned_t(libIRDB::Instruction_t* p_from) : from_instruction(p_from) {}
		libIRDB::Instruction_t* GetInstruction() const { return from_instruction; }
	private:
		libIRDB::Instruction_t* from_instruction;

	friend bool operator< (const UnresolvedPinned_t& lhs, const UnresolvedPinned_t& rhs);
		
};

inline bool operator< (const UnresolvedPinned_t& lhs, const UnresolvedPinned_t& rhs)
{ return lhs.from_instruction->GetBaseID() < rhs.from_instruction->GetBaseID(); }


// an ELF location that needs patching when an Unresolved instrcution
class Patch_t
{
	public:
		Patch_t(RangeAddress_t p_from_addr, UnresolvedType_t p_t) : from_addr(p_from_addr), type(p_t) {}

		RangeAddress_t GetAddress() { return from_addr; }
		UnresolvedType_t GetType() { return type; }

	private:
		RangeAddress_t from_addr;
		UnresolvedType_t type;
};


#endif
