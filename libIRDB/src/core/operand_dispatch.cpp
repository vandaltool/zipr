
#include <libIRDB-core.hpp>
#include <core/operand_csx86.hpp>
#include <core/decode_csx86.hpp>
#include <core/operand_dispatch.hpp>
#include <core/decode_dispatch.hpp>

using namespace std;
using namespace libIRDB;

DecodedOperandDispatcher_t::DecodedOperandDispatcher_t(const shared_ptr<DecodedOperandCapstone_t> copy_cs)  
{
	const auto copy_cs_casted=dynamic_cast<DecodedOperandCapstoneX86_t*>(copy_cs.get());
	cs.reset(new DecodedOperandCapstoneX86_t(*copy_cs_casted));
} 

DecodedOperandDispatcher_t::DecodedOperandDispatcher_t(const DecodedOperandDispatcher_t& copy) 
{
	const auto copy_cs_casted=dynamic_cast<DecodedOperandCapstoneX86_t*>(copy.cs.get());
	cs.reset(new DecodedOperandCapstoneX86_t(*copy_cs_casted));
}

DecodedOperandDispatcher_t::~DecodedOperandDispatcher_t()
{
}

DecodedOperandDispatcher_t& DecodedOperandDispatcher_t::operator=(const DecodedOperandDispatcher_t& copy)
{
	const auto copy_cs_casted=dynamic_cast<DecodedOperandCapstoneX86_t*>(copy.cs.get());
	cs.reset(new DecodedOperandCapstoneX86_t(*copy_cs_casted));
	return *this;
}

#define passthrough_to_cs(ret_type, method_name) \
	ret_type DecodedOperandDispatcher_t::method_name() const \
	{ \
		return cs->method_name(); \
	} 

passthrough_to_cs(bool, isConstant);
passthrough_to_cs(uint64_t, getConstant);
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
