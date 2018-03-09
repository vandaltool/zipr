
#include <libIRDB-core.hpp>
#include <memory>
#include <core/operand_cs.hpp>
#include <core/decode_cs.hpp>

using namespace std;
using namespace libIRDB;

#include <capstone.h>


// static helpers.


static uint32_t to_seg_reg_number(const x86_reg &reg)
{
	switch(reg)
	{
		case X86_REG_ES: return 0;
		case X86_REG_CS: return 1;
		case X86_REG_SS: return 2;
		case X86_REG_DS: return 3;
		case X86_REG_FS: return 4;
		case X86_REG_GS: return 5;
		defaut: assert(0);
	}
	assert(0);
}

static uint32_t to_reg_number(const x86_reg &reg)
{
	switch(reg)
	{	
		case X86_REG_AH: 
		case X86_REG_AL: 
		case X86_REG_AX: 
		case X86_REG_EAX: 
		case X86_REG_RAX:	
			return 0;

		case X86_REG_CH:
		case X86_REG_CL:
		case X86_REG_CX: 
		case X86_REG_ECX: 
		case X86_REG_RCX: 
			return 1;

		case X86_REG_DH: 
		case X86_REG_DX: 
		case X86_REG_DL: 
		case X86_REG_EDX: 
		case X86_REG_RDX:
			return 2;

		case X86_REG_BH: 
		case X86_REG_BL:
		case X86_REG_BX: 
		case X86_REG_EBX: 
		case X86_REG_RBX: 
			return 3;

		case X86_REG_ESP: 
		case X86_REG_RSP:
		case X86_REG_SP:
		case X86_REG_SPL:
			return 4;

		case X86_REG_BP: 
		case X86_REG_BPL: 
		case X86_REG_EBP:
		case X86_REG_RBP: 
			return 5;

		case X86_REG_ESI: 
		case X86_REG_RSI:
		case X86_REG_SI:
		case X86_REG_SIL:
			return 6;

		case X86_REG_DI: 
		case X86_REG_DIL:
		case X86_REG_EDI: 
		case X86_REG_RDI:
			return 7;

		case X86_REG_R8:
		case X86_REG_R8B:
		case X86_REG_R8D:
		case X86_REG_R8W: 
			return 8;

		case X86_REG_R9:
		case X86_REG_R9B:
		case X86_REG_R9D:
		case X86_REG_R9W: 
			return 9;

		case X86_REG_R10:
		case X86_REG_R10B:
		case X86_REG_R10D:
		case X86_REG_R10W:
			return 10;

		case X86_REG_R11:
		case X86_REG_R11B:
		case X86_REG_R11D:
		case X86_REG_R11W: 
			return 11;

		case X86_REG_R12:
		case X86_REG_R12B:
		case X86_REG_R12D:
		case X86_REG_R12W: 
			return 12;

		case X86_REG_R13:
		case X86_REG_R13B:
		case X86_REG_R13D:
		case X86_REG_R13W: 
			return 13;

		case X86_REG_R14:
		case X86_REG_R14B:
		case X86_REG_R14D:
		case X86_REG_R14W: 
			return 14;

		case X86_REG_R15:
		case X86_REG_R15B:
		case X86_REG_R15D:
		case X86_REG_R15W:
			return 15;
	}
	assert(0);
}

// methods

//DecodedOperandCapstone_t& DecodedOperandCapstone_t::operator=(const DecodedOperandCapstone_t& copy)
//{
//	return *this;
//}
//
//DecodedOperandCapstone_t::DecodedOperandCapstone_t(const DecodedOperandCapstone_t& copy)
//{
//	*this=copy;
//}

DecodedOperandCapstone_t::DecodedOperandCapstone_t( const shared_ptr<void> & p_my_insn, uint8_t p_op_num)
	:
	my_insn(p_my_insn),
	op_num(p_op_num)
{
	
}

DecodedOperandCapstone_t::~DecodedOperandCapstone_t()
{
}


bool DecodedOperandCapstone_t::isConstant() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);

	return op.type==X86_OP_IMM;
}

uint64_t DecodedOperandCapstone_t::getConstant() const
{
	if(!isConstant()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-constant operand");
	
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return op.imm;
}

string DecodedOperandCapstone_t::getString() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	const auto handle=DecodedInstructionCapstone_t::cs_handle->getHandle();

	switch(op.type)
	{
		case X86_OP_REG: 
			return string(cs_reg_name(handle, op.reg));
		case X86_OP_IMM: 
			return to_string(op.imm);
		case X86_OP_MEM: 
		{
//			if (op.mem.segment != X86_REG_INVALID)
//				ret_val+=cs_reg_name(handle, op.mem.segment) +"  : " ;
			if (op.mem.base == X86_REG_RIP)
			{
				/* convert pc+disp into disp+insn_size. */
				return to_string(op.mem.disp+the_insn->size);
			}
			else
			{
				string ret_val;
				if (op.mem.base != X86_REG_INVALID)
					ret_val+=cs_reg_name(handle, op.mem.base);

				if (op.mem.index != X86_REG_INVALID)
					ret_val+=string(" + ") +cs_reg_name(handle, op.mem.index);

				if (op.mem.scale != 1)
					ret_val+=string(" * ") + to_string(op.mem.scale);

				if (op.mem.disp != 0)
					ret_val+=" + "+ to_string(op.mem.disp);
				return ret_val;
			}
			assert(0);
		}
		case X86_OP_FP:
		case X86_OP_INVALID: 
		default:
			assert(0);
	}
}

bool DecodedOperandCapstone_t::isRegister() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return op.type==X86_OP_REG;
}

bool DecodedOperandCapstone_t::isGeneralPurposeRegister() const
{

	const auto gp_regs=set<x86_reg>({
		X86_REG_AH, X86_REG_AL, X86_REG_AX, X86_REG_BH, X86_REG_BL,
		X86_REG_BP, X86_REG_BPL, X86_REG_BX, X86_REG_CH, X86_REG_CL,
		X86_REG_CX, X86_REG_DH, X86_REG_DI, X86_REG_DIL,
		X86_REG_DL, X86_REG_DX, X86_REG_EAX, X86_REG_EBP,
		X86_REG_EBX, X86_REG_ECX, X86_REG_EDI, X86_REG_EDX, 
		X86_REG_ESI, X86_REG_ESP, X86_REG_RAX,
		X86_REG_RBP, X86_REG_RBX, X86_REG_RCX, X86_REG_RDI, X86_REG_RDX,
		X86_REG_RSI, X86_REG_RSP, X86_REG_SI,
		X86_REG_SIL, X86_REG_SP, X86_REG_SPL, 
		X86_REG_R8, X86_REG_R9, X86_REG_R10, X86_REG_R11,
		X86_REG_R12, X86_REG_R13, X86_REG_R14, X86_REG_R15,
		X86_REG_R8B, X86_REG_R9B, X86_REG_R10B, X86_REG_R11B,
		X86_REG_R12B, X86_REG_R13B, X86_REG_R14B, X86_REG_R15B, X86_REG_R8D,
		X86_REG_R9D, X86_REG_R10D, X86_REG_R11D, X86_REG_R12D, X86_REG_R13D,
		X86_REG_R14D, X86_REG_R15D, X86_REG_R8W, X86_REG_R9W, X86_REG_R10W,
		X86_REG_R11W, X86_REG_R12W, X86_REG_R13W, X86_REG_R14W, X86_REG_R15W
		});

        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return isRegister() &&  gp_regs.find(op.reg)!=end(gp_regs);
}

bool DecodedOperandCapstone_t::isMmxRegister() const
{
	const auto regs=set<x86_reg>({
		X86_REG_MM0, X86_REG_MM1,
		X86_REG_MM2, X86_REG_MM3, X86_REG_MM4, X86_REG_MM5, X86_REG_MM6,
		X86_REG_MM7, X86_REG_R8});
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return isRegister() &&  regs.find(op.reg)!=end(regs);
}

bool DecodedOperandCapstone_t::isFpuRegister() const
{
	const auto regs=set<x86_reg>({
		X86_REG_ST0, X86_REG_ST1, X86_REG_ST2, X86_REG_ST3,
		X86_REG_ST4, X86_REG_ST5, X86_REG_ST6, X86_REG_ST7,
		});
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return isRegister() &&  regs.find(op.reg)!=end(regs);
}

bool DecodedOperandCapstone_t::isSseRegister() const
{
	const auto regs=set<x86_reg>({
		X86_REG_XMM0, X86_REG_XMM1, X86_REG_XMM2, X86_REG_XMM3, X86_REG_XMM4,
		X86_REG_XMM5, X86_REG_XMM6, X86_REG_XMM7, X86_REG_XMM8, X86_REG_XMM9,
		X86_REG_XMM10, X86_REG_XMM11, X86_REG_XMM12, X86_REG_XMM13, X86_REG_XMM14,
		X86_REG_XMM15, X86_REG_XMM16, X86_REG_XMM17, X86_REG_XMM18, X86_REG_XMM19,
		X86_REG_XMM20, X86_REG_XMM21, X86_REG_XMM22, X86_REG_XMM23, X86_REG_XMM24,
		X86_REG_XMM25, X86_REG_XMM26, X86_REG_XMM27, X86_REG_XMM28, X86_REG_XMM29,
		X86_REG_XMM30, X86_REG_XMM31
		});
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return isRegister() &&  regs.find(op.reg)!=end(regs);
}

bool DecodedOperandCapstone_t::isAvxRegister() const
{
	const auto regs=set<x86_reg>({
		X86_REG_YMM0, X86_REG_YMM1, X86_REG_YMM2,
		X86_REG_YMM3, X86_REG_YMM4, X86_REG_YMM5, X86_REG_YMM6, X86_REG_YMM7,
		X86_REG_YMM8, X86_REG_YMM9, X86_REG_YMM10, X86_REG_YMM11, X86_REG_YMM12,
		X86_REG_YMM13, X86_REG_YMM14, X86_REG_YMM15, X86_REG_YMM16, X86_REG_YMM17,
		X86_REG_YMM18, X86_REG_YMM19, X86_REG_YMM20, X86_REG_YMM21, X86_REG_YMM22,
		X86_REG_YMM23, X86_REG_YMM24, X86_REG_YMM25, X86_REG_YMM26, X86_REG_YMM27,
		X86_REG_YMM28, X86_REG_YMM29, X86_REG_YMM30, X86_REG_YMM31,
		});
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return isRegister() &&  regs.find(op.reg)!=end(regs);
}

bool DecodedOperandCapstone_t::isSpecialRegister() const
{
	const auto regs=set<x86_reg>({
		X86_REG_CR1, X86_REG_CR2, X86_REG_CR3, X86_REG_CR4, X86_REG_CR5,
		X86_REG_CR6, X86_REG_CR7, X86_REG_CR8, X86_REG_CR9, X86_REG_CR10,
        	X86_REG_CR11, X86_REG_CR12, X86_REG_CR13, X86_REG_CR14, X86_REG_CR15,
		X86_REG_DR0, X86_REG_DR1, X86_REG_DR2, X86_REG_DR3, X86_REG_DR4,
        	X86_REG_DR5, X86_REG_DR6, X86_REG_DR7,
		});
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return isRegister() &&  regs.find(op.reg)!=end(regs);
}

bool DecodedOperandCapstone_t::isSegmentRegister() const
{
	const auto regs=set<x86_reg>({
		X86_REG_CS,
		X86_REG_DS,
		X86_REG_ES,
		X86_REG_FS,
		X86_REG_GS,

		});
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return isRegister() &&  regs.find(op.reg)!=end(regs);
}



uint32_t DecodedOperandCapstone_t::getRegNumber() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	if(isGeneralPurposeRegister())
		return to_reg_number(op.reg);
	else if(isMmxRegister())
		return op.reg-X86_REG_MM0;
	else if(isFpuRegister())
		return op.reg-X86_REG_ST0;
	else if(isSseRegister())
		return op.reg-X86_REG_XMM0;
	else if(isAvxRegister())
		return op.reg-X86_REG_YMM0;
	else if(isSegmentRegister())
		return to_seg_reg_number(op.reg);
	else
		assert(0);
}

bool DecodedOperandCapstone_t::isMemory() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return op.type==X86_OP_MEM;
}

bool DecodedOperandCapstone_t::hasBaseRegister() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return isMemory() && op.mem.base!=X86_REG_INVALID && op.mem.base!=X86_REG_RIP;
}

bool DecodedOperandCapstone_t::hasIndexRegister() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return isMemory() && op.mem.index!=X86_REG_INVALID;
}

uint32_t DecodedOperandCapstone_t::getBaseRegister() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return to_reg_number((x86_reg)op.mem.base);
}

uint32_t DecodedOperandCapstone_t::getIndexRegister() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return to_reg_number((x86_reg)op.mem.index);
}

uint32_t DecodedOperandCapstone_t::getScaleValue() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return op.mem.scale;
}

bool DecodedOperandCapstone_t::hasMemoryDisplacement() const
{

	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");

	const auto the_insn = static_cast<cs_insn*>(my_insn.get());
	const auto &op = (the_insn->detail->x86.operands[op_num]);

	const auto modrm = the_insn->detail->x86.modrm;
	const auto mod = (modrm >> 6) & 0x3;
	const auto rm = (modrm & 0x7);
	const auto sib = the_insn->detail->x86.sib;
	const auto sib_base = (sib >> 0) & 0x7;
	const auto sib_index = (sib >> 3) & 0x7;

	switch (mod)
	{
		case 0 /* 00 */:
			if (rm == 4 &&	 // indicates SIB is present.
			    sib_base == 0x5 // indicates disp32 when sib present and mod=00.
			   )
				return true;	// abs 32-bit

			if (rm == 5)
				return true;	// pcrel or abs 32-bit depending on if 32-bit or 64-bit arch.

			return false;

		case 1 /* 01 */:
			return true;

		case 2 /* 10 */:
			return true;

		case 3 /* 11 */:
			return false;

		default: 
			assert(0); //unreachable
	}
	assert(0); // unreachable
} // end of DecodedOperandCapstone_t::hasMemoryDisplacement()

virtual_offset_t DecodedOperandCapstone_t::getMemoryDisplacement() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return op.mem.disp;
}

bool DecodedOperandCapstone_t::isPcrel() const
{
	if(!isMemory())	return false;
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
 
	return (op.mem.base==X86_REG_RIP || op.mem.base==X86_REG_EIP || op.mem.base==X86_REG_IP);
}

/* in bytes */
uint32_t DecodedOperandCapstone_t::getMemoryDisplacementEncodingSize() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of non-memory operand");
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);

	const auto modrm=the_insn->detail->x86.modrm;
	const auto mod=(modrm>>6)&0x3;
	const auto rm=(modrm)&0x7;
	const auto sib=the_insn->detail->x86.sib;
	const auto sib_base=(sib>>0)&0x7;
	const auto sib_index=(sib>>3)&0x7;

	switch(mod)
	{
		case 0 /* 00 */:  	
			if(rm==4 &&	 // indicates SIB is present.
			   sib_base==0x5 // indicates disp32 when sib present and mod=00.
			  ) 
				return 4;	// abs 32-bit
				
			if(rm==5)
				return 4;	// pcrel or abs 32-bit depending on if 32-bit or 64-bit arch.

			throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of operand without displacement");

		case 1 /* 01 */: 	return 1;
		case 2 /* 10 */: 	return 4;

		case 3 /* 11 */:
			throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of operand without displacement");
		default: assert(0);
	}
	assert(0);
}

uint32_t DecodedOperandCapstone_t::getArgumentSizeInBytes() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return op.size;
}

uint32_t DecodedOperandCapstone_t::getArgumentSizeInBits() const
{
	return getArgumentSizeInBytes()*8;
}

bool DecodedOperandCapstone_t::hasSegmentRegister() const
{
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return isMemory() && (op.mem.segment != X86_REG_INVALID);

}

uint32_t DecodedOperandCapstone_t::getSegmentRegister() const
{
	if(!isMemory()) throw std::logic_error(string("Cannot ")+__FUNCTION__+"  of operand without memory operand");
        const auto the_insn=static_cast<cs_insn*>(my_insn.get());
        const auto &op = (the_insn->detail->x86.operands[op_num]);
	return to_seg_reg_number((x86_reg)op.mem.segment);
}


set<string> read_only_operand_mnemonics=
	{
		// specal read-only op cases, rest are some form of compare.
		"push",

		// test 
		"test",

		// bit test (but not bt+comp or bt+reset as those have a reg dest.
		"bt",

		// compare 
		"cmp",

		// compare string
		"cmps",
		"cmpsb",
		"cmpsw",
		"cmpsd",
		"cmpsq",

		// float compare [and pop]
		"fcom",
		"fcomp",
		"fcompp",

		// compare floating point values
		"fcomi",
		"fcomip",
		"fucomi",
		"fucomip",


		// logical compare
		"ptest",
		"vptest",

		// scas variants
		"scas",
		"scass",
		"scasb",
		"scasw",
		"scasd",

		// ucomiss -- unordered cmpare scalar single-precision floating-point values and set flags
		"ucomiss",
		"vucomiss",

		// comiss -- unordered cmpare scalar single-precision floating-point values and set flags
		"comiss",
		"vcomiss",

		// comisd "comapre scalar ordered double-preicions floating-point values and set flags
		"comisd",
		"vcomisd",

		// ucomisd "comapre scalar ordered double-preicions floating-point values and set flags
		"ucomisd",
		"vucomisd",

		// packed bit test
		"vtestps",
		"vtestpd"

		// compare packed {double,single}-prec float values 
		"cmppd",
		"vcmppd",
		"cmpps",
		"vcmpps",

		// compare sclar {double,single}-prec float values
		"cmpsd",
		"vcmpsd",
		"cmpss",
		"vcmpss",

		// compare packed data for equal
		"pcmpeqb",
		"vpcmpeqb",
		"pcmpeqw",
		"vpcmpeqw",
		"pcmpeqd",
		"vpcmpeqd",
		"pcmpeqq",
		"vpcmpeqq",

		// packed compare explicit length string, return {index, mask}
		"pcmpestri",
		"vpcmpestri",
		"pcmpestrm",
		"vpcmpestrm",

		// compare packed data for greather than
		"pcompgtb",
		"vpcompgtb",
		"pcompgtw",
		"vpcompgtw",
		"pcompgtd",
		"vpcompgtd",
		"pcompgtq",
		"vpcompgtq",

		// packed compare implicit length string, return index or mask
		"pcmpistri",
		"vpcmpistri",
		"pcmpistrm",
		"vpcmpistrm",

		// test if in transactional region
		"xtest",

	};



bool DecodedOperandCapstone_t::isRead() const
{
	if(op_num!=0)
		return true;

	const auto d=DecodedInstructionCapstone_t(my_insn);
	if(d.isBranch())	
		return true;	

	const auto room_it=read_only_operand_mnemonics.find(d.getMnemonic());
	const auto in_room=(room_it!=end(read_only_operand_mnemonics));
	if(in_room)
		return true;

	if(d.getMnemonic().substr(0,3)=="mov")
		return false;

	// op0 is typically read/write
	return true;

	assert(0);
}

bool DecodedOperandCapstone_t::isWritten() const
{	
	if(op_num!=0)
		return false;
	const auto d=DecodedInstructionCapstone_t(my_insn);
	if(d.isBranch())	
		return false;	

	const auto room_it=read_only_operand_mnemonics.find(d.getMnemonic());
	const auto in_room=(room_it!=end(read_only_operand_mnemonics));
	if(in_room)
		return false;

	return true;
}
