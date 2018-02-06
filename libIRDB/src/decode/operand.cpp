
#include <libIRDB-decode.hpp>
#include <bea_deprecated.hpp>

using namespace std;
using namespace libIRDB;

// static functions 
static uint32_t beaRegNoToIRDBRegNo(uint32_t regno)
{
	switch(regno)
	{
		case 0: return -1;	// no reg -> -1
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
DecodedOperand_t::DecodedOperand_t(void* arg_voidptr)
{
	ARGTYPE *arg_ptr=static_cast<ARGTYPE*>(arg_voidptr);
	arg_data=(void*)new ARGTYPE(*arg_ptr);
}

DecodedOperand_t::DecodedOperand_t(const DecodedOperand_t& copy)
{
	ARGTYPE *arg_ptr=static_cast<ARGTYPE*>(copy.arg_data);
	arg_data=(void*)new ARGTYPE(*arg_ptr);
}

DecodedOperand_t::~DecodedOperand_t()
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	delete t;
	arg_data=NULL;
}

DecodedOperand_t& DecodedOperand_t::operator=(const DecodedOperand_t& copy)
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	delete t;
	arg_data=NULL;
	ARGTYPE *arg_ptr=static_cast<ARGTYPE*>(copy.arg_data);
	arg_data=(void*)new ARGTYPE(*arg_ptr);

	return *this;
}

bool DecodedOperand_t::isConstant() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	assert(t);
	return (t->ArgType & CONSTANT_TYPE)!=0;
}

string DecodedOperand_t::getString() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	assert(t);
	return t->ArgMnemonic;
}

		
bool DecodedOperand_t::isWrite() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return t->AccessMode==WRITE;
}

bool DecodedOperand_t::isRegister() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return (t->ArgType&REGISTER_TYPE)==REGISTER_TYPE;
}



uint32_t DecodedOperand_t::getRegNumber() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	if(!isRegister())
		throw std::logic_error("getRegNumber called on not register");
	const auto regno=(t->ArgType)&0xFFFF;

	return beaRegNoToIRDBRegNo(regno);
}

bool DecodedOperand_t::isMemory() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return (t->ArgType&MEMORY_TYPE)==MEMORY_TYPE;
}

bool DecodedOperand_t::hasBaseRegister() const
{
	if(!isMemory())
		throw std::logic_error("hasBaseRegister called on not memory operand");

	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return t->Memory.BaseRegister!=0;
}

bool DecodedOperand_t::hasIndexRegister() const
{
	if(!isMemory())
		throw std::logic_error("hasIndexRegister called on not memory operand");

	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return t->Memory.IndexRegister!=0;
}

uint32_t DecodedOperand_t::getBaseRegister() const
{
	if(!isMemory() || !hasBaseRegister())
		throw std::logic_error("getBaseRegister called on not memory operand");

	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return beaRegNoToIRDBRegNo(t->Memory.BaseRegister);
}

uint32_t DecodedOperand_t::getIndexRegister() const
{
	if(!isMemory() || !hasIndexRegister())
		throw std::logic_error("getIndexRegister called on not memory operand");

	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return beaRegNoToIRDBRegNo(t->Memory.IndexRegister);
}

uint32_t DecodedOperand_t::getScaleValue() const
{
	if(!isMemory())
		throw std::logic_error("getScaleValue called on not memory operand");

	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return t->Memory.Scale; 	/* 0 indicates no scale */
}

virtual_offset_t DecodedOperand_t::getMemoryDisplacement() const
{
	if(!isMemory())
		throw std::logic_error("GetBaseRegister called on not memory operand");
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return (virtual_offset_t)t->Memory.Displacement;
}

bool DecodedOperand_t::isPcrel() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return (t->ArgType&RELATIVE_)==RELATIVE_;
}

uint32_t DecodedOperand_t::getMemoryDisplacementEncodingSize() const
{
	ARGTYPE *t=static_cast<ARGTYPE*>(arg_data);
	return t->Memory.DisplacementSize;
}
