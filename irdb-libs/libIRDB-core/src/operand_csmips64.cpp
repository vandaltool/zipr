
#include <libIRDB-core.hpp>
#include <memory>
#include <decode_base.hpp>
#include <decode_csmips.hpp>
#include <operand_base.hpp>
#include <operand_csmips.hpp>

#if 0
using namespace std;
using namespace libIRDB;

#include <capstone.h>
static const auto MIPS64_REG_PC=(mips64_reg)(MIPS64_REG_ENDING+1);


DecodedOperandCapstoneMIPS64_t::DecodedOperandCapstoneMIPS64_t( const shared_ptr<void> & p_my_insn, uint8_t p_op_num)
	: DecodedOperandCapstoneMIPS_t(p_my_insn,p_op_num)
{
	
}

DecodedOperandCapstoneMIPS64_t::~DecodedOperandCapstoneMIPS64_t()
{
}


bool DecodedOperandCapstoneMIPS64_t::isConstant() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips64.operands[op_num]);
	return op.type==MIPS64_OP_IMM;
}

uint64_t DecodedOperandCapstoneMIPS64_t::getConstant() const
{
	if(!isConstant()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-constant operand");
	
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips64.operands[op_num]);
	return op.imm;
}

string DecodedOperandCapstoneMIPS64_t::getString() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips64.operands[op_num]);
        const auto handle=DecodedInstructionCapstoneMIPS64_t::cs_handle->getHandle();

        switch(op.type)
        {
                case MIPS64_OP_REG:
                        return string(cs_reg_name(handle, op.reg));
                case MIPS64_OP_REG_MRS:
                case MIPS64_OP_REG_MSR:
                case MIPS64_OP_FP:
			return string("fpcr");
                case MIPS64_OP_IMM:
                        return to_string(op.imm);
                case MIPS64_OP_MEM:
                {
			string ret_val;
			if (op.mem.base != MIPS64_REG_INVALID)
				ret_val+=cs_reg_name(handle, op.mem.base);

			if (op.mem.index != MIPS64_REG_INVALID)
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

bool DecodedOperandCapstoneMIPS64_t::isRegister() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips64.operands[op_num]);
	return op.type==MIPS64_OP_REG;
}

bool DecodedOperandCapstoneMIPS64_t::isGeneralPurposeRegister() const
{
	const auto gp_regs=set<mips64_reg>({
		MIPS64_REG_X29, MIPS64_REG_X30, MIPS64_REG_SP,  MIPS64_REG_WSP, MIPS64_REG_WZR, MIPS64_REG_XZR,
		MIPS64_REG_W0,  MIPS64_REG_W1,  MIPS64_REG_W2,  MIPS64_REG_W3,  MIPS64_REG_W4,  MIPS64_REG_W5,  MIPS64_REG_W6,  MIPS64_REG_W7,
        	MIPS64_REG_W8,  MIPS64_REG_W9,  MIPS64_REG_W10, MIPS64_REG_W11, MIPS64_REG_W12, MIPS64_REG_W13, MIPS64_REG_W14, MIPS64_REG_W15,
        	MIPS64_REG_W16, MIPS64_REG_W17, MIPS64_REG_W18, MIPS64_REG_W19, MIPS64_REG_W20, MIPS64_REG_W21, MIPS64_REG_W22, MIPS64_REG_W23, 
		MIPS64_REG_W24, MIPS64_REG_W25, MIPS64_REG_W26, MIPS64_REG_W27, MIPS64_REG_W28, MIPS64_REG_W29, MIPS64_REG_W30, 
		MIPS64_REG_X0,  MIPS64_REG_X1,  MIPS64_REG_X2,  MIPS64_REG_X3,  MIPS64_REG_X4,  MIPS64_REG_X5,  MIPS64_REG_X6,  MIPS64_REG_X7,
		MIPS64_REG_X8,  MIPS64_REG_X9,  MIPS64_REG_X10, MIPS64_REG_X11, MIPS64_REG_X12, MIPS64_REG_X13, MIPS64_REG_X14, MIPS64_REG_X15,
		MIPS64_REG_X16, MIPS64_REG_X17, MIPS64_REG_X18, MIPS64_REG_X19, MIPS64_REG_X20, MIPS64_REG_X21, MIPS64_REG_X22, MIPS64_REG_X23,
		MIPS64_REG_X24, MIPS64_REG_X25, MIPS64_REG_X26, MIPS64_REG_X27, MIPS64_REG_X28,
	});

        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips64.operands[op_num]);
        return isRegister() &&  gp_regs.find(op.reg)!=end(gp_regs);

}

bool DecodedOperandCapstoneMIPS64_t::isMmxRegister() const
{
	return false;
}

bool DecodedOperandCapstoneMIPS64_t::isFpuRegister() const
{
	const auto fpu_regs=set<mips64_reg>({
		MIPS64_REG_B0,  MIPS64_REG_B1,  MIPS64_REG_B2,  MIPS64_REG_B3,  MIPS64_REG_B4,  MIPS64_REG_B5,  MIPS64_REG_B6,  MIPS64_REG_B7,
        	MIPS64_REG_B8,  MIPS64_REG_B9,  MIPS64_REG_B10, MIPS64_REG_B11, MIPS64_REG_B12, MIPS64_REG_B13, MIPS64_REG_B14, MIPS64_REG_B15,
        	MIPS64_REG_B16, MIPS64_REG_B17, MIPS64_REG_B18, MIPS64_REG_B19, MIPS64_REG_B20, MIPS64_REG_B21, MIPS64_REG_B22, MIPS64_REG_B23,
        	MIPS64_REG_B24, MIPS64_REG_B25, MIPS64_REG_B26, MIPS64_REG_B27, MIPS64_REG_B28, MIPS64_REG_B29, MIPS64_REG_B30, MIPS64_REG_B31,
		MIPS64_REG_H0,  MIPS64_REG_H1,  MIPS64_REG_H2,  MIPS64_REG_H3,  MIPS64_REG_H4,  MIPS64_REG_H5,  MIPS64_REG_H6,  MIPS64_REG_H7, 
		MIPS64_REG_H8,  MIPS64_REG_H9,  MIPS64_REG_H10, MIPS64_REG_H11, MIPS64_REG_H12, MIPS64_REG_H13, MIPS64_REG_H14, MIPS64_REG_H15, 
		MIPS64_REG_H16, MIPS64_REG_H17, MIPS64_REG_H18, MIPS64_REG_H19, MIPS64_REG_H20, MIPS64_REG_H21, MIPS64_REG_H22, MIPS64_REG_H23, 
		MIPS64_REG_H24, MIPS64_REG_H25, MIPS64_REG_H26, MIPS64_REG_H27, MIPS64_REG_H28, MIPS64_REG_H29, MIPS64_REG_H30, MIPS64_REG_H31, 
	        MIPS64_REG_D0,  MIPS64_REG_D1,  MIPS64_REG_D2,  MIPS64_REG_D3,  MIPS64_REG_D4,  MIPS64_REG_D5,  MIPS64_REG_D6,  MIPS64_REG_D7,
        	MIPS64_REG_D8,  MIPS64_REG_D9,  MIPS64_REG_D10, MIPS64_REG_D11, MIPS64_REG_D12, MIPS64_REG_D13, MIPS64_REG_D14, MIPS64_REG_D15,
		MIPS64_REG_D16, MIPS64_REG_D17, MIPS64_REG_D18, MIPS64_REG_D19, MIPS64_REG_D20, MIPS64_REG_D21, MIPS64_REG_D22, MIPS64_REG_D23,
		MIPS64_REG_D24, MIPS64_REG_D25, MIPS64_REG_D26, MIPS64_REG_D27, MIPS64_REG_D28, MIPS64_REG_D29, MIPS64_REG_D30, MIPS64_REG_D31,
	        MIPS64_REG_H0,  MIPS64_REG_H1,  MIPS64_REG_H2,  MIPS64_REG_H3,  MIPS64_REG_H4,  MIPS64_REG_H5,  MIPS64_REG_H6,  MIPS64_REG_H7,
		MIPS64_REG_H8,  MIPS64_REG_H9,  MIPS64_REG_H10, MIPS64_REG_H11, MIPS64_REG_H12, MIPS64_REG_H13, MIPS64_REG_H14, MIPS64_REG_H15,
		MIPS64_REG_H16, MIPS64_REG_H17, MIPS64_REG_H18, MIPS64_REG_H19, MIPS64_REG_H20, MIPS64_REG_H21, MIPS64_REG_H22, MIPS64_REG_H23,
		MIPS64_REG_H24, MIPS64_REG_H25, MIPS64_REG_H26, MIPS64_REG_H27, MIPS64_REG_H28, MIPS64_REG_H29, MIPS64_REG_H30, MIPS64_REG_H31,
	        MIPS64_REG_Q0,  MIPS64_REG_Q1,  MIPS64_REG_Q2,  MIPS64_REG_Q3,  MIPS64_REG_Q4,  MIPS64_REG_Q5,  MIPS64_REG_Q6,  MIPS64_REG_Q7,
		MIPS64_REG_Q8,  MIPS64_REG_Q9,  MIPS64_REG_Q10, MIPS64_REG_Q11, MIPS64_REG_Q12, MIPS64_REG_Q13, MIPS64_REG_Q14, MIPS64_REG_Q15,
		MIPS64_REG_Q16, MIPS64_REG_Q17, MIPS64_REG_Q18, MIPS64_REG_Q19, MIPS64_REG_Q20, MIPS64_REG_Q21, MIPS64_REG_Q22, MIPS64_REG_Q23,
		MIPS64_REG_Q24, MIPS64_REG_Q25, MIPS64_REG_Q26, MIPS64_REG_Q27, MIPS64_REG_Q28, MIPS64_REG_Q29, MIPS64_REG_Q30, MIPS64_REG_Q31,
		MIPS64_REG_S0,  MIPS64_REG_S1,  MIPS64_REG_S2,  MIPS64_REG_S3,  MIPS64_REG_S4,  MIPS64_REG_S5,  MIPS64_REG_S6,  MIPS64_REG_S7,
		MIPS64_REG_S8,  MIPS64_REG_S9,  MIPS64_REG_S10, MIPS64_REG_S11, MIPS64_REG_S12, MIPS64_REG_S13, MIPS64_REG_S14, MIPS64_REG_S15,
		MIPS64_REG_S16, MIPS64_REG_S17, MIPS64_REG_S18, MIPS64_REG_S19, MIPS64_REG_S20, MIPS64_REG_S21, MIPS64_REG_S22, MIPS64_REG_S23,
		MIPS64_REG_S24, MIPS64_REG_S25, MIPS64_REG_S26, MIPS64_REG_S27, MIPS64_REG_S28, MIPS64_REG_S29, MIPS64_REG_S30, MIPS64_REG_S31,
	});

        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips64.operands[op_num]);
        return isRegister() &&  fpu_regs.find(op.reg)!=end(fpu_regs);
}

bool DecodedOperandCapstoneMIPS64_t::isSseRegister() const
{
	return false;
}

bool DecodedOperandCapstoneMIPS64_t::isAvxRegister() const
{
	return false;
}

bool DecodedOperandCapstoneMIPS64_t::isZmmRegister() const
{
	return false;
}

bool DecodedOperandCapstoneMIPS64_t::isSpecialRegister() const
{
	const auto special_regs=set<mips64_reg>({
		MIPS64_REG_NZCV,
	});

        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips64.operands[op_num]);
        return isRegister() &&  special_regs.find(op.reg)!=end(special_regs);
	return false;
}

bool DecodedOperandCapstoneMIPS64_t::isSegmentRegister() const
{
	return false;
}



uint32_t DecodedOperandCapstoneMIPS64_t::getRegNumber() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips64.operands[op_num]);
	return op.reg;
}

bool DecodedOperandCapstoneMIPS64_t::isMemory() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips64.operands[op_num]);
        return op.type==MIPS64_OP_MEM;

}

bool DecodedOperandCapstoneMIPS64_t::hasBaseRegister() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips64.operands[op_num]);
        return isMemory() && op.mem.base!=MIPS64_REG_INVALID;

}

bool DecodedOperandCapstoneMIPS64_t::hasIndexRegister() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips64.operands[op_num]);
        return isMemory() && op.mem.index!=MIPS64_REG_INVALID;
}

uint32_t DecodedOperandCapstoneMIPS64_t::getBaseRegister() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips64.operands[op_num]);
        return op.mem.base;
}

uint32_t DecodedOperandCapstoneMIPS64_t::getIndexRegister() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips64.operands[op_num]);
        return op.mem.index;
}

uint32_t DecodedOperandCapstoneMIPS64_t::getScaleValue() const
{
	assert(0);
}

bool DecodedOperandCapstoneMIPS64_t::hasMemoryDisplacement() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips64.operands[op_num]);
        return op.mem.disp!=0;

} // end of DecodedOperandCapstoneMIPS64_t::hasMemoryDisplacement()

virtual_offset_t DecodedOperandCapstoneMIPS64_t::getMemoryDisplacement() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips64.operands[op_num]);
        return op.mem.disp;
}

bool DecodedOperandCapstoneMIPS64_t::isPcrel() const
{
	const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips64.operands[op_num]);

	// covers ldr, ldrsw, prfm
	// note: capstone's reports ldr, ldrsw, and prfm as using an imm, when they actually access memory.
	// jdh fixed this in the IRDB's Disassemble routine
	if(isMemory() && op.mem.base==MIPS64_REG_PC)
		return true;

	const auto mnemonic=string(the_insn->mnemonic);
	const auto is_adr_type= mnemonic=="adr"  || mnemonic=="adrp";
	if(is_adr_type && op.type==MIPS64_OP_IMM)
		return true;

	return false;	 // no PC as general purpose reg.
}

/* in bytes */
uint32_t DecodedOperandCapstoneMIPS64_t::getMemoryDisplacementEncodingSize() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
	assert(0);
}

uint32_t DecodedOperandCapstoneMIPS64_t::getArgumentSizeInBytes() const
{
	return false;
}

uint32_t DecodedOperandCapstoneMIPS64_t::getArgumentSizeInBits() const
{
        return getArgumentSizeInBytes()*8;
}

bool DecodedOperandCapstoneMIPS64_t::hasSegmentRegister() const
{
	return false;
}

uint32_t DecodedOperandCapstoneMIPS64_t::getSegmentRegister() const
{
	throw std::logic_error(string("Cannot ")+__FUNCTION__+"  on MIPS architecture");
}

bool DecodedOperandCapstoneMIPS64_t::isRead() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips64.operands[op_num]);
	return (op.access & CS_AC_READ)!=0;
}

bool DecodedOperandCapstoneMIPS64_t::isWritten() const
{	
	// default: use capstone's advice.
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->mips64.operands[op_num]);
	return (op.access & CS_AC_WRITE)!=0;
}
#endif
