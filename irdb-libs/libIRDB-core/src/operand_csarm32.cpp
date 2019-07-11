
#include <libIRDB-core.hpp>
#include <memory>
#include <decode_base.hpp>
#include <decode_csarm.hpp>
#include <operand_base.hpp>
#include <operand_csarm.hpp>


using namespace std;
using namespace libIRDB;

#include <capstone.h>

DecodedOperandCapstoneARM32_t::DecodedOperandCapstoneARM32_t( const shared_ptr<void> & p_my_insn, uint8_t p_op_num)
	: DecodedOperandCapstoneARM_t(p_my_insn,p_op_num)
{
	
}

DecodedOperandCapstoneARM32_t::~DecodedOperandCapstoneARM32_t()
{
}


bool DecodedOperandCapstoneARM32_t::isConstant() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
	return op.type==ARM_OP_IMM;
}

uint64_t DecodedOperandCapstoneARM32_t::getConstant() const
{
	if(!isConstant()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-constant operand");
	
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
	return op.imm;
}

string DecodedOperandCapstoneARM32_t::getString() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
        const auto handle=DecodedInstructionCapstoneARM32_t::cs_handle->getHandle();

        switch(op.type)
        {
                case ARM_OP_REG:
                        return string(cs_reg_name(handle, op.reg));
                case ARM_OP_IMM:
                        return to_string(op.imm);
                case ARM_OP_MEM:
                {
			string ret_val;
			if (op.mem.base != ARM_REG_INVALID)
				ret_val+=cs_reg_name(handle, op.mem.base);

			if (op.mem.index != ARM_REG_INVALID)
				ret_val+=string(" + ") +cs_reg_name(handle, op.mem.index);

			if (op.mem.disp != 0)
				ret_val+=" + "+ to_string(op.mem.disp);

			ret_val += " lsl/ror? ?? ";

			return ret_val;
		}
                default:
                        assert(0);
        }
}

bool DecodedOperandCapstoneARM32_t::isRegister() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
	return op.type==ARM_OP_REG;
}

bool DecodedOperandCapstoneARM32_t::isGeneralPurposeRegister() const
{
	const auto gp_regs=set<arm_reg>({
		ARM_REG_R0,
		ARM_REG_R1,
		ARM_REG_R2,
		ARM_REG_R3,
		ARM_REG_R4,
		ARM_REG_R5,
		ARM_REG_R6,
		ARM_REG_R7,
		ARM_REG_R8,
		ARM_REG_R9,
		ARM_REG_R10,
		ARM_REG_R11,
		ARM_REG_R12,
		ARM_REG_R13,
		ARM_REG_R14,
		ARM_REG_R15,
		});

        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
        return isRegister() &&  gp_regs.find((arm_reg)op.reg)!=end(gp_regs);

}

bool DecodedOperandCapstoneARM32_t::isMmxRegister() const
{
	return false;
}

bool DecodedOperandCapstoneARM32_t::isFpuRegister() const
{
	const auto fpu_regs=set<arm_reg>({
		ARM_REG_D0,
		ARM_REG_D1,
		ARM_REG_D2,
		ARM_REG_D3,
		ARM_REG_D4,
		ARM_REG_D5,
		ARM_REG_D6,
		ARM_REG_D7,
		ARM_REG_D8,
		ARM_REG_D9,
		ARM_REG_D10,
		ARM_REG_D11,
		ARM_REG_D12,
		ARM_REG_D13,
		ARM_REG_D14,
		ARM_REG_D15,
		ARM_REG_D16,
		ARM_REG_D17,
		ARM_REG_D18,
		ARM_REG_D19,
		ARM_REG_D20,
		ARM_REG_D21,
		ARM_REG_D22,
		ARM_REG_D23,
		ARM_REG_D24,
		ARM_REG_D25,
		ARM_REG_D26,
		ARM_REG_D27,
		ARM_REG_D28,
		ARM_REG_D29,
		ARM_REG_D30,
		ARM_REG_D31,
		ARM_REG_Q0,
		ARM_REG_Q1,
		ARM_REG_Q2,
		ARM_REG_Q3,
		ARM_REG_Q4,
		ARM_REG_Q5,
		ARM_REG_Q6,
		ARM_REG_Q7,
		ARM_REG_Q8,
		ARM_REG_Q9,
		ARM_REG_Q10,
		ARM_REG_Q11,
		ARM_REG_Q12,
		ARM_REG_Q13,
		ARM_REG_Q14,
		ARM_REG_Q15,
		ARM_REG_S1,
		ARM_REG_S2,
		ARM_REG_S3,
		ARM_REG_S4,
		ARM_REG_S5,
		ARM_REG_S6,
		ARM_REG_S7,
		ARM_REG_S8,
		ARM_REG_S9,
		ARM_REG_S10,
		ARM_REG_S11,
		ARM_REG_S12,
		ARM_REG_S13,
		ARM_REG_S14,
		ARM_REG_S15,
		ARM_REG_S16,
		ARM_REG_S17,
		ARM_REG_S18,
		ARM_REG_S19,
		ARM_REG_S20,
		ARM_REG_S21,
		ARM_REG_S22,
		ARM_REG_S23,
		ARM_REG_S24,
		ARM_REG_S25,
		ARM_REG_S26,
		ARM_REG_S27,
		ARM_REG_S28,
		ARM_REG_S29,
		ARM_REG_S30,
		ARM_REG_S31,
		});

        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
        return isRegister() &&  fpu_regs.find((arm_reg)op.reg)!=end(fpu_regs);
}

bool DecodedOperandCapstoneARM32_t::isSseRegister() const
{
	return false;
}

bool DecodedOperandCapstoneARM32_t::isAvxRegister() const
{
	return false;
}

bool DecodedOperandCapstoneARM32_t::isZmmRegister() const
{
	return false;
}

bool DecodedOperandCapstoneARM32_t::isSpecialRegister() const
{
	const auto special_regs=set<arm_reg>({
		ARM_REG_APSR,
		ARM_REG_APSR_NZCV,
		ARM_REG_CPSR,
		ARM_REG_FPEXC,
		ARM_REG_FPINST,
		ARM_REG_FPSCR,
		ARM_REG_FPSCR_NZCV,
		ARM_REG_FPSID,
		ARM_REG_ITSTATE,
		ARM_REG_PC,
		ARM_REG_SPSR,
		ARM_REG_FPINST2,
		ARM_REG_MVFR0,
		ARM_REG_MVFR1,
		ARM_REG_MVFR2,
	});

        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
        return isRegister() &&  special_regs.find((arm_reg)op.reg)!=end(special_regs);
	return false;
}

bool DecodedOperandCapstoneARM32_t::isSegmentRegister() const
{
	return false;
}



uint32_t DecodedOperandCapstoneARM32_t::getRegNumber() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
	return op.reg;
}

bool DecodedOperandCapstoneARM32_t::isMemory() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
        return op.type==ARM_OP_MEM;

}

bool DecodedOperandCapstoneARM32_t::hasBaseRegister() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
        return isMemory() && op.mem.base!=ARM_REG_INVALID;

}

bool DecodedOperandCapstoneARM32_t::hasIndexRegister() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
        return isMemory() && op.mem.index!=ARM_REG_INVALID;
}

uint32_t DecodedOperandCapstoneARM32_t::getBaseRegister() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
        return op.mem.base;
}

uint32_t DecodedOperandCapstoneARM32_t::getIndexRegister() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
        return op.mem.index;
}

uint32_t DecodedOperandCapstoneARM32_t::getScaleValue() const
{
	assert(0);
}

bool DecodedOperandCapstoneARM32_t::hasMemoryDisplacement() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
        return op.mem.disp!=0;

} // end of DecodedOperandCapstoneARM32_t::hasMemoryDisplacement()

virtual_offset_t DecodedOperandCapstoneARM32_t::getMemoryDisplacement() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
        return op.mem.disp;
}

bool DecodedOperandCapstoneARM32_t::isPcrel() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);

	// covers ldr, ldrsw, prfm
	// note: capstone's reports ldr, ldrsw, and prfm as using an imm, when they actually access memory.
	// jdh fixed this in the IRDB's Disassemble routine
	if(isMemory() && op.mem.base==ARM_REG_PC)
		return true;

	return isRegister() && getRegNumber()==ARM_REG_PC; // check to see if it's actually the pc

}

/* in bytes */
uint32_t DecodedOperandCapstoneARM32_t::getMemoryDisplacementEncodingSize() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
	assert(0);
}

uint32_t DecodedOperandCapstoneARM32_t::getArgumentSizeInBytes() const
{
	return false;
}

uint32_t DecodedOperandCapstoneARM32_t::getArgumentSizeInBits() const
{
        return getArgumentSizeInBytes()*8;
}

bool DecodedOperandCapstoneARM32_t::hasSegmentRegister() const
{
	return false;
}

uint32_t DecodedOperandCapstoneARM32_t::getSegmentRegister() const
{
	throw std::logic_error(string("Cannot ")+__FUNCTION__+"  on ARM architecture");
}

bool DecodedOperandCapstoneARM32_t::isRead() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
	return (op.access & CS_AC_READ)!=0;
}

bool DecodedOperandCapstoneARM32_t::isWritten() const
{	
	// default: use capstone's advice.
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm.operands[op_num]);
	return (op.access & CS_AC_WRITE)!=0;
}
