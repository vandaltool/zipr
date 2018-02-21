
#include <libIRDB-core.hpp>
#include <bea_deprecated.hpp>

using namespace std;
using namespace libIRDB;

// static functions 
static uint32_t beaRegNoToIRDBRegNo(uint32_t regno)
{
	switch(regno)
	{
		case 0: return -1;	// no reg -> -1
		case REG0+REG2: /* eax:edx pair -- call it reg 0 */ return 0;
		case REG0: return 0;
		case REG1: return 1;
		case REG2: return 2;
		case REG3: return 3;
		case REG4: return 4;
		case REG5: return 5;
		case REG6: return 6;
		case REG7: return 7;
		case REG8: return 8;
		case REG9: return 9;
		case REG10: return 10;
		case REG11: return 11;
		case REG12: return 12;
		case REG13: return 13;
		case REG14: return 14;
		case REG15: return 15;
		default: assert(0); // odd number from BEA?
	};
}


// methods
DecodedOperandBea_t::DecodedOperandBea_t(void* arg_voidptr)
{
	ARGTYPE *arg_ptr=static_cast<ARGTYPE*>(arg_voidptr);
	arg_data=(void*)new ARGTYPE(*arg_ptr);
}

DecodedOperandBea_t::DecodedOperandBea_t(const DecodedOperandBea_t& copy)
{
	ARGTYPE *arg_ptr=static_cast<ARGTYPE*>(copy.arg_data);
	arg_data=(void*)new ARGTYPE(*arg_ptr);
}

DecodedOperandBea_t::~DecodedOperandBea_t()
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	delete t;
	arg_data=NULL;
}

DecodedOperandBea_t& DecodedOperandBea_t::operator=(const DecodedOperandBea_t& copy)
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	delete t;
	arg_data=NULL;
	ARGTYPE *arg_ptr=static_cast<ARGTYPE*>(copy.arg_data);
	arg_data=(void*)new ARGTYPE(*arg_ptr);

	return *this;
}

bool DecodedOperandBea_t::isConstant() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	assert(t);
	return (t->ArgType & CONSTANT_TYPE)!=0;
}

string DecodedOperandBea_t::getString() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	assert(t);
	return t->ArgMnemonic;
}

		
bool DecodedOperandBea_t::isWrite() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return t->AccessMode==WRITE;
}

bool DecodedOperandBea_t::isRegister() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return (t->ArgType&REGISTER_TYPE)==REGISTER_TYPE;
}

bool DecodedOperandBea_t::isGeneralPurposeRegister() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return isRegister() && (t->ArgType&GENERAL_REG)==GENERAL_REG;
}

bool DecodedOperandBea_t::isMmxRegister() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return isRegister() && (t->ArgType&MMX_REG)==MMX_REG;
}

bool DecodedOperandBea_t::isFpuRegister() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return isRegister() && (t->ArgType&FPU_REG)==FPU_REG;
}

bool DecodedOperandBea_t::isSseRegister() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return isRegister() && (t->ArgType&SSE_REG)==SSE_REG;
}

bool DecodedOperandBea_t::isAvxRegister() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return isRegister() && (t->ArgType&AVX_REG)==AVX_REG;
}

bool DecodedOperandBea_t::isSpecialRegister() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return isRegister() && (t->ArgType&SPECIAL_REG)==SPECIAL_REG;
}

bool DecodedOperandBea_t::isSegmentRegister() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return isRegister() && (t->ArgType&SEGMENT_REG)==SEGMENT_REG;
}

uint32_t DecodedOperandBea_t::getRegNumber() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	if(!isRegister())
		throw std::logic_error("getRegNumber called on not register");
	const auto regno=(t->ArgType)&0xFFFF;

	return beaRegNoToIRDBRegNo(regno);
}

bool DecodedOperandBea_t::isMemory() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return (t->ArgType&MEMORY_TYPE)==MEMORY_TYPE;
}

bool DecodedOperandBea_t::hasBaseRegister() const
{
	if(!isMemory())
		throw std::logic_error("hasBaseRegister called on not memory operand");

	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return t->Memory.BaseRegister!=0;
}

bool DecodedOperandBea_t::hasIndexRegister() const
{
	if(!isMemory())
		throw std::logic_error("hasIndexRegister called on not memory operand");

	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return t->Memory.IndexRegister!=0;
}

uint32_t DecodedOperandBea_t::getBaseRegister() const
{
	if(!isMemory() || !hasBaseRegister())
		throw std::logic_error("getBaseRegister called on not memory operand");

	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return beaRegNoToIRDBRegNo(t->Memory.BaseRegister);
}

uint32_t DecodedOperandBea_t::getIndexRegister() const
{
	if(!isMemory() || !hasIndexRegister())
		throw std::logic_error("getIndexRegister called on not memory operand");

	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return beaRegNoToIRDBRegNo(t->Memory.IndexRegister);
}

uint32_t DecodedOperandBea_t::getScaleValue() const
{
	if(!isMemory())
		throw std::logic_error("getScaleValue called on not memory operand");

	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return t->Memory.Scale; 	/* 0 indicates no scale */
}

bool DecodedOperandBea_t::hasMemoryDisplacement() const
{
	if(!isMemory())
		throw std::logic_error("GetBaseRegister called on not memory operand");
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return t->Memory.DisplacementAddr!=0;
}

virtual_offset_t DecodedOperandBea_t::getMemoryDisplacement() const
{
	if(!isMemory())
		throw std::logic_error("GetBaseRegister called on not memory operand");
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return (virtual_offset_t)t->Memory.Displacement;
}

bool DecodedOperandBea_t::isPcrel() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return (t->ArgType&RELATIVE_)==RELATIVE_;
}

uint32_t DecodedOperandBea_t::getMemoryDisplacementEncodingSize() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return t->Memory.DisplacementSize;
}

uint32_t DecodedOperandBea_t::getArgumentSizeInBytes() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return t->ArgSize/8;
}

uint32_t DecodedOperandBea_t::getArgumentSizeInBits() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return t->ArgSize;
}

bool DecodedOperandBea_t::hasSegmentRegister() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return t->SegmentReg!=0;

}

uint32_t DecodedOperandBea_t::getSegmentRegister() const
{
	if(!hasSegmentRegister())
		throw std::logic_error("getSegmentRegisterNumber called with no segment register");
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return t->SegmentReg;
}

bool DecodedOperandBea_t::isRead() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return (t->AccessMode&READ)==READ;
}

bool DecodedOperandBea_t::isWritten() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return (t->AccessMode&WRITE)==WRITE;
}
