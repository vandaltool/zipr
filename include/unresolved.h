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
