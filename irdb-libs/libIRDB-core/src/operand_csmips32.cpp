
#include <libIRDB-core.hpp>
#include <memory>
#include <decode_base.hpp>
#include <decode_csmips.hpp>
#include <operand_base.hpp>
#include <operand_csmips.hpp>


using namespace std;
using namespace libIRDB;

#include <capstone.h>

DecodedOperandCapstoneMIPS32_t::DecodedOperandCapstoneMIPS32_t( const shared_ptr<void> & p_my_insn, uint8_t p_op_num)
	: DecodedOperandCapstoneMIPS_t(p_my_insn,p_op_num)
{
	
}

DecodedOperandCapstoneMIPS32_t::~DecodedOperandCapstoneMIPS32_t()
{
}


bool DecodedOperandCapstoneMIPS32_t::isConstant() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips.operands[op_num]);
	return op.type==MIPS_OP_IMM;
}

uint64_t DecodedOperandCapstoneMIPS32_t::getConstant() const
{
	if(!isConstant()) throw logic_error(string("Cannot ")+__FUNCTION__+"  of non-constant operand");
	
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips.operands[op_num]);
	return op.imm;
}

string DecodedOperandCapstoneMIPS32_t::getString() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips.operands[op_num]);
        const auto handle=DecodedInstructionCapstoneMIPS32_t::cs_handle->getHandle();

        switch(op.type)
        {
                case MIPS_OP_REG:
                        return string(cs_reg_name(handle, op.reg));
                case MIPS_OP_IMM:
                        return to_string(op.imm);
                case MIPS_OP_MEM:
                {
			string ret_val;
			if (op.mem.base != MIPS_REG_INVALID)
				ret_val+=cs_reg_name(handle, op.mem.base);

			if (op.mem.disp != 0)
				ret_val+=" + "+ to_string(op.mem.disp);

			ret_val += " lsl/ror? ?? ";

			return ret_val;
		}
                default:
                        assert(0);
        }
}

bool DecodedOperandCapstoneMIPS32_t::isRegister() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips.operands[op_num]);
	return op.type==MIPS_OP_REG;
}

bool DecodedOperandCapstoneMIPS32_t::isGeneralPurposeRegister() const
{
	const auto gp_regs=set<mips_reg>({
		MIPS_REG_0,
		MIPS_REG_1,
		MIPS_REG_2,
		MIPS_REG_3,
		MIPS_REG_4,
		MIPS_REG_5,
		MIPS_REG_6,
		MIPS_REG_7,
		MIPS_REG_8,
		MIPS_REG_9,
		MIPS_REG_10,
		MIPS_REG_11,
		MIPS_REG_12,
		MIPS_REG_13,
		MIPS_REG_14,
		MIPS_REG_15,
		MIPS_REG_16,
		MIPS_REG_17,
		MIPS_REG_18,
		MIPS_REG_19,
		MIPS_REG_20,
		MIPS_REG_21,
		MIPS_REG_22,
		MIPS_REG_23,
		MIPS_REG_24,
		MIPS_REG_25,
		MIPS_REG_26,
		MIPS_REG_27,
		MIPS_REG_28,
		MIPS_REG_29,
		MIPS_REG_30,
		MIPS_REG_31,
		});

        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips.operands[op_num]);
        return isRegister() &&  gp_regs.find((mips_reg)op.reg)!=end(gp_regs);

}

bool DecodedOperandCapstoneMIPS32_t::isMmxRegister() const
{
	return false;
}

bool DecodedOperandCapstoneMIPS32_t::isFpuRegister() const
{
	const auto fpu_regs=set<mips_reg>({
		MIPS_REG_F0,
		MIPS_REG_F1,
		MIPS_REG_F2,
		MIPS_REG_F3,
		MIPS_REG_F4,
		MIPS_REG_F5,
		MIPS_REG_F6,
		MIPS_REG_F7,
		MIPS_REG_F8,
		MIPS_REG_F9,
		MIPS_REG_F10,
		MIPS_REG_F11,
		MIPS_REG_F12,
		MIPS_REG_F13,
		MIPS_REG_F14,
		MIPS_REG_F15,
		MIPS_REG_F16,
		MIPS_REG_F17,
		MIPS_REG_F18,
		MIPS_REG_F19,
		MIPS_REG_F20,
		MIPS_REG_F21,
		MIPS_REG_F22,
		MIPS_REG_F23,
		MIPS_REG_F24,
		MIPS_REG_F25,
		MIPS_REG_F26,
		MIPS_REG_F27,
		MIPS_REG_F28,
		MIPS_REG_F29,
		MIPS_REG_F30,
		MIPS_REG_F31,
		});

        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips.operands[op_num]);
        return isRegister() &&  fpu_regs.find((mips_reg)op.reg)!=end(fpu_regs);
}

bool DecodedOperandCapstoneMIPS32_t::isSseRegister() const
{
	return false;
}

bool DecodedOperandCapstoneMIPS32_t::isAvxRegister() const
{
	return false;
}

bool DecodedOperandCapstoneMIPS32_t::isZmmRegister() const
{
	return false;
}

bool DecodedOperandCapstoneMIPS32_t::isSpecialRegister() const
{
	const auto special_regs=set<mips_reg>({
	        MIPS_REG_AC0,
		MIPS_REG_AC1,
		MIPS_REG_AC2,
		MIPS_REG_AC3,
		MIPS_REG_CC0,
		MIPS_REG_CC1,
		MIPS_REG_CC2,
		MIPS_REG_CC3,
		MIPS_REG_CC4,
		MIPS_REG_CC5,
		MIPS_REG_CC6,
		MIPS_REG_CC7,

	});

        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips.operands[op_num]);
        return isRegister() &&  special_regs.find((mips_reg)op.reg)!=end(special_regs);
	return false;
}

bool DecodedOperandCapstoneMIPS32_t::isSegmentRegister() const
{
	return false;
}



uint32_t DecodedOperandCapstoneMIPS32_t::getRegNumber() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips.operands[op_num]);
	return op.reg;
}

bool DecodedOperandCapstoneMIPS32_t::isMemory() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips.operands[op_num]);
        return op.type==MIPS_OP_MEM;

}

bool DecodedOperandCapstoneMIPS32_t::hasBaseRegister() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips.operands[op_num]);
        return isMemory() && op.mem.base!=MIPS_REG_INVALID;

}

bool DecodedOperandCapstoneMIPS32_t::hasIndexRegister() const
{
	return false;
}

uint32_t DecodedOperandCapstoneMIPS32_t::getBaseRegister() const
{
	if(!isMemory()) throw logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips.operands[op_num]);
        return op.mem.base;
}

uint32_t DecodedOperandCapstoneMIPS32_t::getIndexRegister() const
{
	throw logic_error(string("Cannot ")+__FUNCTION__+"  of operand with no index register");
}

uint32_t DecodedOperandCapstoneMIPS32_t::getScaleValue() const
{
	assert(0);
}

bool DecodedOperandCapstoneMIPS32_t::hasMemoryDisplacement() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips.operands[op_num]);
        return op.mem.disp!=0;

} // end of DecodedOperandCapstoneMIPS32_t::hasMemoryDisplacement()

virtual_offset_t DecodedOperandCapstoneMIPS32_t::getMemoryDisplacement() const
{
	if(!isMemory()) throw logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips.operands[op_num]);
        return op.mem.disp;
}

bool DecodedOperandCapstoneMIPS32_t::isPcrel() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips.operands[op_num]);

	// covers ldr, ldrsw, prfm
	// note: capstone's reports ldr, ldrsw, and prfm as using an imm, when they actually access memory.
	// jdh fixed this in the IRDB's Disassemble routine
	if(isMemory() && op.mem.base==MIPS_REG_PC)
		return true;

	return isRegister() && getRegNumber()==MIPS_REG_PC; // check to see if it's actually the pc

}

/* in bytes */
uint32_t DecodedOperandCapstoneMIPS32_t::getMemoryDisplacementEncodingSize() const
{
	if(!isMemory()) throw logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
	assert(0);
}

uint32_t DecodedOperandCapstoneMIPS32_t::getArgumentSizeInBytes() const
{
	return false;
}

uint32_t DecodedOperandCapstoneMIPS32_t::getArgumentSizeInBits() const
{
        return getArgumentSizeInBytes()*8;
}

bool DecodedOperandCapstoneMIPS32_t::hasSegmentRegister() const
{
	return false;
}

uint32_t DecodedOperandCapstoneMIPS32_t::getSegmentRegister() const
{
	throw logic_error(string("Cannot ")+__FUNCTION__+"  on MIPS architecture");
}

bool DecodedOperandCapstoneMIPS32_t::isRead() const
{
        //const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        // const auto &op = (the_insn->detail->mips.operands[op_num]);
	// return (op.access & CS_AC_READ)!=0;
	return op_num > 0;
}

bool DecodedOperandCapstoneMIPS32_t::isWritten() const
{	
	// default: use capstone's advice.
        //const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        //const auto &op = (the_insn->detail->mips.operands[op_num]);
	//return (op.access & CS_AC_WRITE)!=0;
	return op_num == 0;
}
