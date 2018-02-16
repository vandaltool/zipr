
#include <libIRDB-decode.hpp>

using namespace std;
using namespace libIRDB;

DecodedOperandMeta_t::DecodedOperandMeta_t(const DecodedOperandBea_t& copy_bea)  :
	bea(copy_bea)
{
} 

DecodedOperandMeta_t::DecodedOperandMeta_t(const DecodedOperandMeta_t& copy) : 
	bea(copy.bea)
{
}

DecodedOperandMeta_t::~DecodedOperandMeta_t()
{
}

DecodedOperandMeta_t& DecodedOperandMeta_t::operator=(const DecodedOperandMeta_t& copy)
{
	bea=copy.bea;
	return *this;
}

#define passthrough_to_bea(ret_type, method_name) \
	ret_type DecodedOperandMeta_t::method_name() const \
	{ \
		return bea.method_name(); \
	} 

passthrough_to_bea(bool, isConstant);
passthrough_to_bea(string, getString);
passthrough_to_bea(bool, isWrite);
passthrough_to_bea(bool, isRegister);
passthrough_to_bea(bool, isGeneralPurposeRegister);
passthrough_to_bea(bool, isMmxRegister);
passthrough_to_bea(bool, isFpuRegister);
passthrough_to_bea(bool, isSseRegister);
passthrough_to_bea(bool, isAvxRegister);
passthrough_to_bea(bool, isSpecialRegister);
passthrough_to_bea(bool, isSegmentRegister);
passthrough_to_bea(uint32_t, getRegNumber);
passthrough_to_bea(bool, isMemory);
passthrough_to_bea(bool, hasBaseRegister);
passthrough_to_bea(bool, hasIndexRegister);
passthrough_to_bea(uint32_t, getBaseRegister);
passthrough_to_bea(uint32_t, getIndexRegister);
passthrough_to_bea(uint32_t, getScaleValue);
passthrough_to_bea(bool, hasMemoryDisplacement);
passthrough_to_bea(virtual_offset_t, getMemoryDisplacement);
passthrough_to_bea(bool, isPcrel);
passthrough_to_bea(uint32_t, getMemoryDisplacementEncodingSize);
passthrough_to_bea(uint32_t, getArgumentSizeInBytes);
passthrough_to_bea(uint32_t, getArgumentSizeInBits);
passthrough_to_bea(bool, hasSegmentRegister);
passthrough_to_bea(uint32_t, getSegmentRegister);
passthrough_to_bea(bool, isRead);
passthrough_to_bea(bool, isWritten);
