
#include <libIRDB-core.hpp>
#include <memory>
#include <core/operand_csx86.hpp>
#include <core/decode_csx86.hpp>

using namespace std;
using namespace libIRDB;

#include <capstone.h>



DecodedOperandCapstoneARM_t::DecodedOperandCapstoneARM_t( const shared_ptr<void> & p_my_insn, uint8_t p_op_num)
	:
	my_insn(p_my_insn),
	op_num(p_op_num)
{
	
}

DecodedOperandCapstoneARM_t::~DecodedOperandCapstoneARM_t()
{
}


bool DecodedOperandCapstoneARM_t::isConstant() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);

	return op.type==ARM_OP_IMM;
}

uint64_t DecodedOperandCapstoneARM_t::getConstant() const
{
	if(!isConstant()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-constant operand");
	
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
	return op.imm;
}

string DecodedOperandCapstoneARM_t::getString() const
{
	assert(0);
}

bool DecodedOperandCapstoneARM_t::isRegister() const
{
	assert(0);
}

bool DecodedOperandCapstoneARM_t::isGeneralPurposeRegister() const
{
	assert(0);
}

bool DecodedOperandCapstoneARM_t::isMmxRegister() const
{
	return false;
}

bool DecodedOperandCapstoneARM_t::isFpuRegister() const
{
	return false;
}

bool DecodedOperandCapstoneARM_t::isSseRegister() const
{
	return false;
}

bool DecodedOperandCapstoneARM_t::isAvxRegister() const
{
	return false;
}

bool DecodedOperandCapstoneARM_t::isZmmRegister() const
{
	return false;
}

bool DecodedOperandCapstoneARM_t::isSpecialRegister() const
{
	return false;
}

bool DecodedOperandCapstoneARM_t::isSegmentRegister() const
{
	return false;
}



uint32_t DecodedOperandCapstoneARM_t::getRegNumber() const
{
	assert(0);
}

bool DecodedOperandCapstoneARM_t::isMemory() const
{
	assert(0);
}

bool DecodedOperandCapstoneARM_t::hasBaseRegister() const
{
	assert(0);
}

bool DecodedOperandCapstoneARM_t::hasIndexRegister() const
{
	assert(0);
}

uint32_t DecodedOperandCapstoneARM_t::getBaseRegister() const
{
	assert(0);
}

uint32_t DecodedOperandCapstoneARM_t::getIndexRegister() const
{
	assert(0);
}

uint32_t DecodedOperandCapstoneARM_t::getScaleValue() const
{
	assert(0);
}

bool DecodedOperandCapstoneARM_t::hasMemoryDisplacement() const
{
	assert(0);

} // end of DecodedOperandCapstoneARM_t::hasMemoryDisplacement()

virtual_offset_t DecodedOperandCapstoneARM_t::getMemoryDisplacement() const
{
	assert(0);
}

bool DecodedOperandCapstoneARM_t::isPcrel() const
{
	assert(0);
}

/* in bytes */
uint32_t DecodedOperandCapstoneARM_t::getMemoryDisplacementEncodingSize() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
	assert(0);
}

uint32_t DecodedOperandCapstoneARM_t::getArgumentSizeInBytes() const
{
	assert(0);
}

uint32_t DecodedOperandCapstoneARM_t::getArgumentSizeInBits() const
{
	assert(0);
}

bool DecodedOperandCapstoneARM_t::hasSegmentRegister() const
{
	return false;
}

uint32_t DecodedOperandCapstoneARM_t::getSegmentRegister() const
{
	assert(0);
}

bool DecodedOperandCapstoneARM_t::isRead() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
	return (op.access & CS_AC_READ)!=0;
}

bool DecodedOperandCapstoneARM_t::isWritten() const
{	
	// default: use capstone's advice.
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return (op.access & CS_AC_WRITE)!=0;
}
