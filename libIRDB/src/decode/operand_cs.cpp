
#include <libIRDB-decode.hpp>

using namespace std;
using namespace libIRDB;

#include <capstone.h>


// methods

DecodedOperandCapstone_t& DecodedOperandCapstone_t::operator=(const DecodedOperandCapstone_t& copy)
{
	return *this;
}

DecodedOperandCapstone_t::DecodedOperandCapstone_t(const DecodedOperandCapstone_t& copy)
{
	*this=copy;
}

DecodedOperandCapstone_t::~DecodedOperandCapstone_t()
{
}


bool DecodedOperandCapstone_t::isConstant() const
{
	return false;
}

string DecodedOperandCapstone_t::getString() const
{
	return "";
}

		
bool DecodedOperandCapstone_t::isWrite() const
{
	return false;
}

bool DecodedOperandCapstone_t::isRegister() const
{
	return false;
}

bool DecodedOperandCapstone_t::isGeneralPurposeRegister() const
{
	return false;
}

bool DecodedOperandCapstone_t::isMmxRegister() const
{
	return false;
}

bool DecodedOperandCapstone_t::isFpuRegister() const
{
	return false;
}

bool DecodedOperandCapstone_t::isSseRegister() const
{
	return false;
}

bool DecodedOperandCapstone_t::isAvxRegister() const
{
	return false;
}

bool DecodedOperandCapstone_t::isSpecialRegister() const
{
	return false;
}

bool DecodedOperandCapstone_t::isSegmentRegister() const
{
	return false;
}

uint32_t DecodedOperandCapstone_t::getRegNumber() const
{
	return 0;
}

bool DecodedOperandCapstone_t::isMemory() const
{
	return false;
}

bool DecodedOperandCapstone_t::hasBaseRegister() const
{
	return false;
}

bool DecodedOperandCapstone_t::hasIndexRegister() const
{
	return false;
}

uint32_t DecodedOperandCapstone_t::getBaseRegister() const
{
	return 0;
}

uint32_t DecodedOperandCapstone_t::getIndexRegister() const
{
	return 0;
}

uint32_t DecodedOperandCapstone_t::getScaleValue() const
{
	return 0;
}

bool DecodedOperandCapstone_t::hasMemoryDisplacement() const
{
	return false;
}

virtual_offset_t DecodedOperandCapstone_t::getMemoryDisplacement() const
{
	return 0;
}

bool DecodedOperandCapstone_t::isPcrel() const
{
	return false;
}

uint32_t DecodedOperandCapstone_t::getMemoryDisplacementEncodingSize() const
{
	return 0;
}

uint32_t DecodedOperandCapstone_t::getArgumentSizeInBytes() const
{
	return 0;
}

uint32_t DecodedOperandCapstone_t::getArgumentSizeInBits() const
{
	return 0;
}

bool DecodedOperandCapstone_t::hasSegmentRegister() const
{
	return false;

}

uint32_t DecodedOperandCapstone_t::getSegmentRegister() const
{
}

bool DecodedOperandCapstone_t::isRead() const
{
	return false;
}

bool DecodedOperandCapstone_t::isWritten() const
{
	return false;
}
