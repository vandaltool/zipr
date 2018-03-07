
#include <libIRDB-core.hpp>
#include <core/operand_cs.hpp>
#include <core/decode_cs.hpp>
#include <core/operand_bea.hpp>
#include <core/decode_bea.hpp>
#include <core/operand_meta.hpp>
#include <core/decode_meta.hpp>

using namespace std;
using namespace libIRDB;

DecodedOperandMeta_t::DecodedOperandMeta_t(const DecodedOperandCapstone_t& copy_cs)  :
	cs(copy_cs)
{
} 

DecodedOperandMeta_t::DecodedOperandMeta_t(const DecodedOperandMeta_t& copy) : 
	cs(copy.cs)
{
}

DecodedOperandMeta_t::~DecodedOperandMeta_t()
{
}

DecodedOperandMeta_t& DecodedOperandMeta_t::operator=(const DecodedOperandMeta_t& copy)
{
	cs=copy.cs;
	return *this;
}

#define passthrough_to_cs(ret_type, method_name) \
	ret_type DecodedOperandMeta_t::method_name() const \
	{ \
		return cs.method_name(); \
	} 

passthrough_to_cs(bool, isConstant);
passthrough_to_cs(string, getString);
passthrough_to_cs(bool, isRegister);
passthrough_to_cs(bool, isGeneralPurposeRegister);
passthrough_to_cs(bool, isMmxRegister);
passthrough_to_cs(bool, isFpuRegister);
passthrough_to_cs(bool, isSseRegister);
passthrough_to_cs(bool, isAvxRegister);
passthrough_to_cs(bool, isSpecialRegister);
passthrough_to_cs(bool, isSegmentRegister);
passthrough_to_cs(uint32_t, getRegNumber);
passthrough_to_cs(bool, isMemory);
passthrough_to_cs(bool, hasBaseRegister);
passthrough_to_cs(bool, hasIndexRegister);
passthrough_to_cs(uint32_t, getBaseRegister);
passthrough_to_cs(uint32_t, getIndexRegister);
passthrough_to_cs(uint32_t, getScaleValue);
passthrough_to_cs(bool, hasMemoryDisplacement);
passthrough_to_cs(virtual_offset_t, getMemoryDisplacement);
passthrough_to_cs(bool, isPcrel);
passthrough_to_cs(uint32_t, getMemoryDisplacementEncodingSize);
passthrough_to_cs(uint32_t, getArgumentSizeInBytes);
passthrough_to_cs(uint32_t, getArgumentSizeInBits);
passthrough_to_cs(bool, hasSegmentRegister);
passthrough_to_cs(uint32_t, getSegmentRegister);
passthrough_to_cs(bool, isRead);
passthrough_to_cs(bool, isWritten);
