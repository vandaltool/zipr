
#include <libIRDB-core.hpp>
#include <memory>
#include <decode_base.hpp>
#include <decode_csarm.hpp>
#include <operand_base.hpp>
#include <operand_csarm.hpp>


using namespace std;
using namespace libIRDB;

#include <capstone.h>
static const auto ARM64_REG_PC=(arm64_reg)(ARM64_REG_ENDING+1);


DecodedOperandCapstoneARM64_t::DecodedOperandCapstoneARM64_t( const shared_ptr<void> & p_my_insn, uint8_t p_op_num)
	: DecodedOperandCapstoneARM_t(p_my_insn,p_op_num)
{
	
}

DecodedOperandCapstoneARM64_t::~DecodedOperandCapstoneARM64_t()
{
}


bool DecodedOperandCapstoneARM64_t::isConstant() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm64.operands[op_num]);
	return op.type==ARM64_OP_IMM;
}

uint64_t DecodedOperandCapstoneARM64_t::getConstant() const
{
	if(!isConstant()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-constant operand");
	
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm64.operands[op_num]);
	return op.imm;
}

string DecodedOperandCapstoneARM64_t::getString() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm64.operands[op_num]);
        const auto handle=DecodedInstructionCapstoneARM64_t::cs_handle->getHandle();

        switch(op.type)
        {
                case ARM64_OP_REG:
                        return string(cs_reg_name(handle, op.reg));
                case ARM64_OP_REG_MRS:
                case ARM64_OP_REG_MSR:
                case ARM64_OP_FP:
			return string("fpcr");
                case ARM64_OP_IMM:
                        return to_string(op.imm);
                case ARM64_OP_MEM:
                {
			string ret_val;
			if (op.mem.base != ARM64_REG_INVALID)
				ret_val+=cs_reg_name(handle, op.mem.base);

			if (op.mem.index != ARM64_REG_INVALID)
				ret_val+=string(" + ") +cs_reg_name(handle, op.mem.index);

			if (op.mem.disp != 0)
				ret_val+=" + "+ to_string(op.mem.disp);

			if(ret_val=="")
				return "0";

			return ret_val;
		}
                default:
                        assert(0);
        }
}

bool DecodedOperandCapstoneARM64_t::isRegister() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm64.operands[op_num]);
	return op.type==ARM64_OP_REG;
}

bool DecodedOperandCapstoneARM64_t::isGeneralPurposeRegister() const
{
	const auto gp_regs=set<arm64_reg>({
		ARM64_REG_X29, ARM64_REG_X30, ARM64_REG_SP,  ARM64_REG_WSP, ARM64_REG_WZR, ARM64_REG_XZR,
		ARM64_REG_W0,  ARM64_REG_W1,  ARM64_REG_W2,  ARM64_REG_W3,  ARM64_REG_W4,  ARM64_REG_W5,  ARM64_REG_W6,  ARM64_REG_W7,
        	ARM64_REG_W8,  ARM64_REG_W9,  ARM64_REG_W10, ARM64_REG_W11, ARM64_REG_W12, ARM64_REG_W13, ARM64_REG_W14, ARM64_REG_W15,
        	ARM64_REG_W16, ARM64_REG_W17, ARM64_REG_W18, ARM64_REG_W19, ARM64_REG_W20, ARM64_REG_W21, ARM64_REG_W22, ARM64_REG_W23, 
		ARM64_REG_W24, ARM64_REG_W25, ARM64_REG_W26, ARM64_REG_W27, ARM64_REG_W28, ARM64_REG_W29, ARM64_REG_W30, 
		ARM64_REG_X0,  ARM64_REG_X1,  ARM64_REG_X2,  ARM64_REG_X3,  ARM64_REG_X4,  ARM64_REG_X5,  ARM64_REG_X6,  ARM64_REG_X7,
		ARM64_REG_X8,  ARM64_REG_X9,  ARM64_REG_X10, ARM64_REG_X11, ARM64_REG_X12, ARM64_REG_X13, ARM64_REG_X14, ARM64_REG_X15,
		ARM64_REG_X16, ARM64_REG_X17, ARM64_REG_X18, ARM64_REG_X19, ARM64_REG_X20, ARM64_REG_X21, ARM64_REG_X22, ARM64_REG_X23,
		ARM64_REG_X24, ARM64_REG_X25, ARM64_REG_X26, ARM64_REG_X27, ARM64_REG_X28,
	});

        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm64.operands[op_num]);
        return isRegister() &&  gp_regs.find(op.reg)!=end(gp_regs);

}

bool DecodedOperandCapstoneARM64_t::isMmxRegister() const
{
	return false;
}

bool DecodedOperandCapstoneARM64_t::isFpuRegister() const
{
	const auto fpu_regs=set<arm64_reg>({
		ARM64_REG_B0,  ARM64_REG_B1,  ARM64_REG_B2,  ARM64_REG_B3,  ARM64_REG_B4,  ARM64_REG_B5,  ARM64_REG_B6,  ARM64_REG_B7,
        	ARM64_REG_B8,  ARM64_REG_B9,  ARM64_REG_B10, ARM64_REG_B11, ARM64_REG_B12, ARM64_REG_B13, ARM64_REG_B14, ARM64_REG_B15,
        	ARM64_REG_B16, ARM64_REG_B17, ARM64_REG_B18, ARM64_REG_B19, ARM64_REG_B20, ARM64_REG_B21, ARM64_REG_B22, ARM64_REG_B23,
        	ARM64_REG_B24, ARM64_REG_B25, ARM64_REG_B26, ARM64_REG_B27, ARM64_REG_B28, ARM64_REG_B29, ARM64_REG_B30, ARM64_REG_B31,
		ARM64_REG_H0,  ARM64_REG_H1,  ARM64_REG_H2,  ARM64_REG_H3,  ARM64_REG_H4,  ARM64_REG_H5,  ARM64_REG_H6,  ARM64_REG_H7, 
		ARM64_REG_H8,  ARM64_REG_H9,  ARM64_REG_H10, ARM64_REG_H11, ARM64_REG_H12, ARM64_REG_H13, ARM64_REG_H14, ARM64_REG_H15, 
		ARM64_REG_H16, ARM64_REG_H17, ARM64_REG_H18, ARM64_REG_H19, ARM64_REG_H20, ARM64_REG_H21, ARM64_REG_H22, ARM64_REG_H23, 
		ARM64_REG_H24, ARM64_REG_H25, ARM64_REG_H26, ARM64_REG_H27, ARM64_REG_H28, ARM64_REG_H29, ARM64_REG_H30, ARM64_REG_H31, 
	        ARM64_REG_D0,  ARM64_REG_D1,  ARM64_REG_D2,  ARM64_REG_D3,  ARM64_REG_D4,  ARM64_REG_D5,  ARM64_REG_D6,  ARM64_REG_D7,
        	ARM64_REG_D8,  ARM64_REG_D9,  ARM64_REG_D10, ARM64_REG_D11, ARM64_REG_D12, ARM64_REG_D13, ARM64_REG_D14, ARM64_REG_D15,
		ARM64_REG_D16, ARM64_REG_D17, ARM64_REG_D18, ARM64_REG_D19, ARM64_REG_D20, ARM64_REG_D21, ARM64_REG_D22, ARM64_REG_D23,
		ARM64_REG_D24, ARM64_REG_D25, ARM64_REG_D26, ARM64_REG_D27, ARM64_REG_D28, ARM64_REG_D29, ARM64_REG_D30, ARM64_REG_D31,
	        ARM64_REG_H0,  ARM64_REG_H1,  ARM64_REG_H2,  ARM64_REG_H3,  ARM64_REG_H4,  ARM64_REG_H5,  ARM64_REG_H6,  ARM64_REG_H7,
		ARM64_REG_H8,  ARM64_REG_H9,  ARM64_REG_H10, ARM64_REG_H11, ARM64_REG_H12, ARM64_REG_H13, ARM64_REG_H14, ARM64_REG_H15,
		ARM64_REG_H16, ARM64_REG_H17, ARM64_REG_H18, ARM64_REG_H19, ARM64_REG_H20, ARM64_REG_H21, ARM64_REG_H22, ARM64_REG_H23,
		ARM64_REG_H24, ARM64_REG_H25, ARM64_REG_H26, ARM64_REG_H27, ARM64_REG_H28, ARM64_REG_H29, ARM64_REG_H30, ARM64_REG_H31,
	        ARM64_REG_Q0,  ARM64_REG_Q1,  ARM64_REG_Q2,  ARM64_REG_Q3,  ARM64_REG_Q4,  ARM64_REG_Q5,  ARM64_REG_Q6,  ARM64_REG_Q7,
		ARM64_REG_Q8,  ARM64_REG_Q9,  ARM64_REG_Q10, ARM64_REG_Q11, ARM64_REG_Q12, ARM64_REG_Q13, ARM64_REG_Q14, ARM64_REG_Q15,
		ARM64_REG_Q16, ARM64_REG_Q17, ARM64_REG_Q18, ARM64_REG_Q19, ARM64_REG_Q20, ARM64_REG_Q21, ARM64_REG_Q22, ARM64_REG_Q23,
		ARM64_REG_Q24, ARM64_REG_Q25, ARM64_REG_Q26, ARM64_REG_Q27, ARM64_REG_Q28, ARM64_REG_Q29, ARM64_REG_Q30, ARM64_REG_Q31,
		ARM64_REG_S0,  ARM64_REG_S1,  ARM64_REG_S2,  ARM64_REG_S3,  ARM64_REG_S4,  ARM64_REG_S5,  ARM64_REG_S6,  ARM64_REG_S7,
		ARM64_REG_S8,  ARM64_REG_S9,  ARM64_REG_S10, ARM64_REG_S11, ARM64_REG_S12, ARM64_REG_S13, ARM64_REG_S14, ARM64_REG_S15,
		ARM64_REG_S16, ARM64_REG_S17, ARM64_REG_S18, ARM64_REG_S19, ARM64_REG_S20, ARM64_REG_S21, ARM64_REG_S22, ARM64_REG_S23,
		ARM64_REG_S24, ARM64_REG_S25, ARM64_REG_S26, ARM64_REG_S27, ARM64_REG_S28, ARM64_REG_S29, ARM64_REG_S30, ARM64_REG_S31,
	});

        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm64.operands[op_num]);
        return isRegister() &&  fpu_regs.find(op.reg)!=end(fpu_regs);
}

bool DecodedOperandCapstoneARM64_t::isSseRegister() const
{
	return false;
}

bool DecodedOperandCapstoneARM64_t::isAvxRegister() const
{
	return false;
}

bool DecodedOperandCapstoneARM64_t::isZmmRegister() const
{
	return false;
}

bool DecodedOperandCapstoneARM64_t::isSpecialRegister() const
{
	const auto special_regs=set<arm64_reg>({
		ARM64_REG_NZCV,
	});

        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm64.operands[op_num]);
        return isRegister() &&  special_regs.find(op.reg)!=end(special_regs);
	return false;
}

bool DecodedOperandCapstoneARM64_t::isSegmentRegister() const
{
	return false;
}



uint32_t DecodedOperandCapstoneARM64_t::getRegNumber() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm64.operands[op_num]);
	return op.reg;
}

bool DecodedOperandCapstoneARM64_t::isMemory() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm64.operands[op_num]);
        return op.type==ARM64_OP_MEM;

}

bool DecodedOperandCapstoneARM64_t::hasBaseRegister() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm64.operands[op_num]);
        return isMemory() && op.mem.base!=ARM64_REG_INVALID;

}

bool DecodedOperandCapstoneARM64_t::hasIndexRegister() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm64.operands[op_num]);
        return isMemory() && op.mem.index!=ARM64_REG_INVALID;
}

uint32_t DecodedOperandCapstoneARM64_t::getBaseRegister() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm64.operands[op_num]);
        return op.mem.base;
}

uint32_t DecodedOperandCapstoneARM64_t::getIndexRegister() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm64.operands[op_num]);
        return op.mem.index;
}

uint32_t DecodedOperandCapstoneARM64_t::getScaleValue() const
{
	assert(0);
}

bool DecodedOperandCapstoneARM64_t::hasMemoryDisplacement() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm64.operands[op_num]);
        return op.mem.disp!=0;

} // end of DecodedOperandCapstoneARM64_t::hasMemoryDisplacement()

virtual_offset_t DecodedOperandCapstoneARM64_t::getMemoryDisplacement() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm64.operands[op_num]);
        return op.mem.disp;
}

bool DecodedOperandCapstoneARM64_t::isPcrel() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm64.operands[op_num]);

	// covers ldr, ldrsw, prfm
	// note: capstone's reports ldr, ldrsw, and prfm as using an imm, when they actually access memory.
	// jdh fixed this in the IRDB's Disassemble routine
	if(isMemory() && op.mem.base==ARM64_REG_PC)
		return true;

	const auto mnemonic=string(the_insn->mnemonic);
	const auto is_adr_type= mnemonic=="adr"  || mnemonic=="adrp";
	if(is_adr_type && op.type==ARM64_OP_IMM)
		return true;

	return false;	 // no PC as general purpose reg.
}

/* in bytes */
uint32_t DecodedOperandCapstoneARM64_t::getMemoryDisplacementEncodingSize() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
	assert(0);
}

uint32_t DecodedOperandCapstoneARM64_t::getArgumentSizeInBytes() const
{
	return false;
}

uint32_t DecodedOperandCapstoneARM64_t::getArgumentSizeInBits() const
{
        return getArgumentSizeInBytes()*8;
}

bool DecodedOperandCapstoneARM64_t::hasSegmentRegister() const
{
	return false;
}

uint32_t DecodedOperandCapstoneARM64_t::getSegmentRegister() const
{
	throw std::logic_error(string("Cannot ")+__FUNCTION__+"  on ARM architecture");
}

bool DecodedOperandCapstoneARM64_t::isRead() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm64.operands[op_num]);
	return (op.access & CS_AC_READ)!=0;
}

bool DecodedOperandCapstoneARM64_t::isWritten() const
{	
	// default: use capstone's advice.
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->arm64.operands[op_num]);
	return (op.access & CS_AC_WRITE)!=0;
}
